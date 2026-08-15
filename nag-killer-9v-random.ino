/*
    ================================================================
    nag_echo v3 — runtime-configurable CAN counter-echo
                   with on-board WiFi dashboard
    ================================================================

    Educational / research firmware. NOT for use on public roads.
    Hard safety cap: torque is clamped to ±1.80 Nm in firmware,
    cannot be overridden from the dashboard.

Arudino Board Setup for Waveshare ESP32-S3-RS485-CAN
https://www.waveshare.com/wiki/ESP32-S3-RS485-CAN#Arduino_Project_Parameter_Setting
ESP32 S3 Dev Module
USB CDC On Boot Disabled
Flash Size 16MB
Partition Scheme: 16M Flash (3BM App/9.9MB FATFS)
PSRAM: OPI PSRAM

*/

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <nvs_flash.h>
#include <esp_system.h>
#include "driver/twai.h"
#include "index_html.h"

#include <ElegantOTASync.h>            // From Arduino library manager, install "ElegantOTA by Ayush Sharma", v3 or higher, but modified for Sync use

#define CAN_TX_PIN    16			// 5 for SN65HVD230 or ATOMIC CANBus Base, 16 for SIT1050T Waveshare ESP32-S3-RS4850-CAN
#define CAN_RX_PIN    15			// 6 for SN65HVD230 or ATOMIC CANBus Base, 15 for SIT1050T Waveshare ESP32-S3-RS4850-CAN

// ── Safety hard caps (NOT user-overridable) ─────────────────────
static const uint16_t TORQUE_RAW_MAX = 0x8B6;
static const uint16_t TORQUE_RAW_MIN = 0x74E;
static const float    TORQUE_NM_MAX  = +1.80f;
static const float    TORQUE_NM_MIN  = -1.80f;
static const uint8_t  MAX_TORQUE_ENTRIES = 8;

// ── Timing constants ────────────────────────────────────────────
static const unsigned long DRIVER_WAKE_DELAY_MS = 10000;  // #1: Before CAN init
static const unsigned long INJECTION_DELAY_MS = 15000;    // After CAN init

// ── Modes ───────────────────────────────────────────────────────
enum NagMode : uint8_t { MODE_A = 0, MODE_B = 1, MODE_CUSTOM = 2, MODE_C = 3 };

// ── Runtime config (persisted to NVS) ───────────────────────────
struct Config {
  bool     enabled;
  uint8_t  mode;
  uint16_t targetId;
  uint8_t  torqueCount;
  uint8_t  torqueB2[MAX_TORQUE_ENTRIES];
  uint8_t  torqueB3[MAX_TORQUE_ENTRIES];
  uint8_t  hoRatePct;
  uint16_t burstMs;
  uint16_t pauseMs;
  uint16_t apStateId;
  uint8_t  apStateByte;
  uint8_t  apStateShift;
  uint8_t  apStateMask;
  uint8_t  handsOnByte;
  uint8_t  handsOnShift;
  uint8_t  handsOnMask;
  uint16_t steeringId;
  uint8_t  steeringByteHi;
  uint8_t  steeringByteLo;
  float    steeringScale;
  float    steeringOffset;
};

static Config cfg;
static portMUX_TYPE cfgMux = portMUX_INITIALIZER_UNLOCKED;

// ── Live context ───────────────────────────────────────────────
struct Context {
  uint8_t  apState;
  uint8_t  handsOnState;
  uint8_t  prevHandsOnState;
  float    steeringAngleDeg;
  unsigned long lastApStateMs;
  unsigned long lastSteeringMs;
};
static Context ctx;
static portMUX_TYPE ctxMux = portMUX_INITIALIZER_UNLOCKED;

// ── Stats ───────────────────────────────────────────────────────
static volatile uint32_t rxFrames    = 0;
static volatile uint32_t echoCount   = 0;
static volatile uint32_t txOk        = 0;
static volatile uint32_t txFail      = 0;
static volatile uint32_t echoLatUs   = 0;
static volatile uint8_t  realHo      = 0;
static volatile float    realTorque  = 0;
static volatile uint8_t  lastInjectedHo = 0;
static volatile float    lastInjectedNm = 0;
static unsigned long bootTime = 0;
static unsigned long canInitTime = 0;  // When TWAI actually started
static volatile bool twaiReady = false;  // True only after TWAI starts cleanly

static volatile uint32_t canAnyFrames = 0;
static volatile unsigned long lastCanFrameMs = 0;  // Last time any CAN frame was received
static unsigned long lastCanLogMs = 0;
static unsigned long lastStatusLog = 0;
static unsigned long lastNoCanWarn = 0;  // #2: For log throttling
static unsigned long lastTxFailLog = 0;  // Throttle TX fail logs

static uint8_t previousB3 = 0xA7;        // midpoint B3 for torque values between 0x98 and 0xB6 (1.48nm - 1.78nm)

// ── Heartbeat counters (#4) ─────────────────────────────────────
static volatile uint32_t canBeat = 0;
static volatile uint32_t canRxBeat = 0;
static volatile uint32_t webBeat = 0;

// ── RTC boot count (#5) ─────────────────────────────────────────
RTC_DATA_ATTR uint32_t rtcBootCount = 0;

static const char* resetReasonName(esp_reset_reason_t r) {
  switch (r) {
    case ESP_RST_POWERON:   return "POWERON";
    case ESP_RST_EXT:       return "EXTERNAL_RESET";
    case ESP_RST_SW:        return "SOFTWARE_RESET";
    case ESP_RST_PANIC:     return "PANIC";
    case ESP_RST_INT_WDT:   return "INT_WDT";
    case ESP_RST_TASK_WDT:  return "TASK_WDT";
    case ESP_RST_WDT:       return "OTHER_WDT";
    case ESP_RST_DEEPSLEEP: return "DEEPSLEEP";
    case ESP_RST_BROWNOUT:  return "BROWNOUT";
    case ESP_RST_SDIO:      return "SDIO";
    default:                return "UNKNOWN";
  }
}

// ── Persistence ─────────────────────────────────────────────────
static Preferences prefs;

static void cfgSetCommonDefaults(Config& c) {
  c.enabled        = true;
  c.burstMs        = 2300;    // was 1000
  c.pauseMs        = 1500;
  c.apStateId      = 0x399;
  c.apStateByte    = 0;
  c.apStateShift   = 4;
  c.apStateMask    = 0x0F;
  c.handsOnByte    = 0;
  c.handsOnShift   = 0;
  c.handsOnMask    = 0x0F;
  c.steeringId     = 0x129;
  c.steeringByteHi = 1;
  c.steeringByteLo = 0;
  c.steeringScale  = 0.1f;
  c.steeringOffset = 0.0f;
}

static void cfgDefaultsModeA(Config& c) {
  cfgSetCommonDefaults(c);
  c.mode        = MODE_A;
  c.targetId    = 0x370;
  c.torqueCount = 1;
  c.torqueB2[0] = 0x08;
  c.torqueB3[0] = 0xB6;
  c.hoRatePct   = 100;
}
static void cfgDefaultsModeB(Config& c) {
  cfgSetCommonDefaults(c);
  c.mode        = MODE_B;
  c.targetId    = 0x370;
  c.torqueCount = 8;
  c.torqueB2[0] = 0x08; c.torqueB3[0] = 0xB6;
  c.torqueB2[1] = 0x08; c.torqueB3[1] = 0xAC;
  c.torqueB2[2] = 0x08; c.torqueB3[2] = 0xB4;
  c.torqueB2[3] = 0x08; c.torqueB3[3] = 0xA2;
  c.torqueB2[4] = 0x08; c.torqueB3[4] = 0xB5;
  c.torqueB2[5] = 0x08; c.torqueB3[5] = 0xB6;
  c.torqueB2[6] = 0x08; c.torqueB3[6] = 0xAF;
  c.torqueB2[7] = 0x08; c.torqueB3[7] = 0xB5;
  c.hoRatePct   = 100;
}
static void cfgDefaultsModeC(Config& c) {
  cfgSetCommonDefaults(c);
  c.mode        = MODE_C;
  c.targetId    = 0x370;
  c.torqueCount = 8;
  c.torqueB2[0] = 0x08; c.torqueB3[0] = 0xB6;
  c.torqueB2[1] = 0x08; c.torqueB3[1] = 0xAC;
  c.torqueB2[2] = 0x08; c.torqueB3[2] = 0xA2;
  c.torqueB2[3] = 0x08; c.torqueB3[3] = 0xB4;
  c.torqueB2[4] = 0x08; c.torqueB3[4] = 0xAF;
  c.torqueB2[5] = 0x08; c.torqueB3[5] = 0xB6;
  c.torqueB2[6] = 0x08; c.torqueB3[6] = 0xB1;
  c.torqueB2[7] = 0x08; c.torqueB3[7] = 0xB5;
  c.hoRatePct   = 100;
}
static void clampTorque(uint8_t& b2, uint8_t& b3) {
  uint16_t raw = ((b2 & 0x0F) << 8) | b3;
  if (raw > TORQUE_RAW_MAX) raw = TORQUE_RAW_MAX;
  if (raw < TORQUE_RAW_MIN) raw = TORQUE_RAW_MIN;
  b2 = (b2 & 0xF0) | ((raw >> 8) & 0x0F);
  b3 = raw & 0xFF;
}

static void nmToBytes(float nm, uint8_t& b2lo, uint8_t& b3) {
  if (nm > TORQUE_NM_MAX) nm = TORQUE_NM_MAX;
  if (nm < TORQUE_NM_MIN) nm = TORQUE_NM_MIN;
  uint16_t raw = (uint16_t)((nm + 20.5f) * 100.0f + 0.5f);
  if (raw > TORQUE_RAW_MAX) raw = TORQUE_RAW_MAX;
  if (raw < TORQUE_RAW_MIN) raw = TORQUE_RAW_MIN;
  b2lo = (raw >> 8) & 0x0F;
  b3   = raw & 0xFF;
}

static void cfgClampAll(Config& c) {
  if (c.torqueCount < 1) c.torqueCount = 1;
  if (c.torqueCount > MAX_TORQUE_ENTRIES) c.torqueCount = MAX_TORQUE_ENTRIES;
  if (c.hoRatePct > 100) c.hoRatePct = 100;
  if (c.burstMs < 50)    c.burstMs   = 50;
  if (c.burstMs > 10000) c.burstMs   = 10000;
  if (c.pauseMs > 10000) c.pauseMs   = 10000;
  for (uint8_t i = 0; i < c.torqueCount; i++) clampTorque(c.torqueB2[i], c.torqueB3[i]);
}

static void cfgLoad() {
  Serial.println("NVS: Loading config...");
  
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    Serial.println("NVS: Corrupted, erasing...");
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  
  if (err != ESP_OK) {
    Serial.printf("NVS: Init failed %d, using defaults\n", err);
    cfgDefaultsModeA(cfg);
    return;
  }
  
  if (!prefs.begin("nag", true)) {
    Serial.println("NVS: No existing config, using defaults");
    cfgDefaultsModeA(cfg);
    return;
  }
  
  if (!prefs.isKey("v")) {
    prefs.end();
    cfgDefaultsModeA(cfg);
    return;
  }
  
  cfgSetCommonDefaults(cfg);
  cfg.enabled        = prefs.getBool("en", true);
  cfg.mode           = prefs.getUChar("mode", 0);
  cfg.targetId       = prefs.getUShort("id", 0x370);
  cfg.torqueCount    = prefs.getUChar("tc", 1);
  
  size_t n = prefs.getBytes("tb2", cfg.torqueB2, MAX_TORQUE_ENTRIES);
  if (n == 0) { cfg.torqueB2[0] = 0x08; }
  n = prefs.getBytes("tb3", cfg.torqueB3, MAX_TORQUE_ENTRIES);
  if (n == 0) { cfg.torqueB3[0] = 0xB6; }
  
  cfg.hoRatePct      = prefs.getUChar("ho", 100);
  cfg.burstMs        = prefs.getUShort("bms", 1000);
  cfg.pauseMs        = prefs.getUShort("pms", 1500);
  cfg.apStateId      = prefs.getUShort("apid", 0x399);
  cfg.steeringId     = prefs.getUShort("stid", 0x129);
  prefs.end();
  
  cfgClampAll(cfg);
  Serial.println("NVS: Config loaded OK");
}

static void cfgSave() {
  cfgClampAll(cfg);
  
  if (!prefs.begin("nag", false)) {
    Serial.println("NVS: Save failed - could not open");
    return;
  }
  
  prefs.putBool("en",     cfg.enabled);
  prefs.putUChar("mode",  cfg.mode);
  prefs.putUShort("id",   cfg.targetId);
  prefs.putUChar("tc",    cfg.torqueCount);
  prefs.putBytes("tb2",   cfg.torqueB2, MAX_TORQUE_ENTRIES);
  prefs.putBytes("tb3",   cfg.torqueB3, MAX_TORQUE_ENTRIES);
  prefs.putUChar("ho",    cfg.hoRatePct);
  prefs.putUShort("bms",  cfg.burstMs);
  prefs.putUShort("pms",  cfg.pauseMs);
  prefs.putUShort("apid", cfg.apStateId);
  prefs.putUShort("stid", cfg.steeringId);
  prefs.putUChar("v",     2);
  prefs.end();

}

// Adding random walk torque variation between 1.48-1.78nm (0x0898 - 0x08B6, or 0x98 - 0xB6 for B3)
static void update_torqueB3(void)
{
    int step = (rand() % 11) - 5;   // -5 .. +5
    int value = (int)previousB3 + step;

    if (value < 0x98)       // corresponds to 1.48nm (might need to increase)
        value = 0x98;
    else if (value > 0xB6)  // corresponds to 1.78nm (limit)
        value = 0xB6;

    previousB3 = (uint8_t)value;
}


static bool decideInjection(const twai_message_t& src,
                            uint8_t& out_b2, uint8_t& out_b3, bool& out_setHo) {
  if (src.data_length_code < 8) return false;
  
  unsigned long now = millis();

  uint8_t  mode, tCount, hoPct;
  uint16_t burstMs, pauseMs;
  uint8_t  tB2[MAX_TORQUE_ENTRIES], tB3[MAX_TORQUE_ENTRIES];
  
  portENTER_CRITICAL(&cfgMux);
  mode    = cfg.mode;
  tCount  = cfg.torqueCount;
  hoPct   = cfg.hoRatePct;
  burstMs = cfg.burstMs;
  pauseMs = cfg.pauseMs;
  for (uint8_t i = 0; i < tCount; i++) { 
    tB2[i] = cfg.torqueB2[i]; 
    tB3[i] = cfg.torqueB3[i]; 
  }
  portEXIT_CRITICAL(&cfgMux);

  static uint8_t  tIdx = 0;
  static uint16_t hoSeq = 0;
  static uint32_t lastChangeMs = 0;
  static uint8_t  prevMode = 0xFF;

  // Keep mode-change detection local to injection logic.
  // The old global tracker was updated during cfgSave(), so live mode
  // switches could be saved before this function ever saw the change.
  if (mode != prevMode) {
    tIdx = 0;
    hoSeq = 0;
    lastChangeMs = now;
    prevMode = mode;
  }

  if (mode == MODE_A || mode == MODE_CUSTOM) {
    out_b2 = tB2[tIdx % tCount];
    out_b3 = tB3[tIdx % tCount];
    tIdx++;
    bool setHo = ((hoSeq * 100u) / 65536u < (uint16_t)hoPct);
    hoSeq = (uint16_t)(hoSeq * 1103u + 12345u);
    out_setHo = setHo;
    return true;
  }

  if (mode == MODE_B) {
    uint32_t cycleMs = (uint32_t)burstMs + (uint32_t)pauseMs;
    if (cycleMs == 0) cycleMs = 1;
    uint32_t phase = (uint32_t)(now - bootTime) % cycleMs;
    if (phase >= burstMs) return false;
    
    if (now - lastChangeMs >= 200) { 
      tIdx = (tIdx + 1) % tCount; 
      lastChangeMs = now; 
    }
    out_b2 = tB2[tIdx];
    out_b3 = tB3[tIdx];
    out_setHo = true;
    return true;
  }

  if (mode == MODE_C) {
    uint32_t cycleMs = (uint32_t)burstMs + (uint32_t)pauseMs;
    if (cycleMs == 0) cycleMs = 1;
    uint32_t phase = (uint32_t)(now - bootTime) % cycleMs;
    if (phase >= burstMs) return false;
    
    if (now - lastChangeMs >= 200) { 
      update_torqueB3(); 
      lastChangeMs = now; 
    }
    out_b2 = 0x08;
    out_b3 = previousB3;
    out_setHo = true;
    return true;
  }

  return false;
}

static void echoModified(const twai_message_t& src) {
  if (src.data_length_code < 8) return;
  
  uint8_t b2 = 0, b3 = 0; 
  bool setHo = false;
  if (!decideInjection(src, b2, b3, setHo)) return;

  twai_message_t e;
  e.identifier        = src.identifier;
  e.data_length_code  = src.data_length_code;
  e.flags             = 0;
  e.data[0] = src.data[0];
  e.data[1] = src.data[1];
  e.data[2] = (src.data[2] & 0xF0) | (b2 & 0x0F);
  e.data[3] = b3;
  e.data[4] = setHo ? (src.data[4] | 0x40) : src.data[4];
  e.data[5] = src.data[5];
  e.data[6] = (src.data[6] & 0xF0) | (((src.data[6] & 0x0F) + 1) & 0x0F);
  
  uint16_t s = e.data[0] + e.data[1] + e.data[2] + e.data[3]
             + e.data[4] + e.data[5] + e.data[6];
  e.data[7] = (uint8_t)((s + 0x73) & 0xFF);

  unsigned long t0 = micros();
  // Keep original 2ms transmit wait; boot/power fixes should not change known-good TX behavior.
  esp_err_t err = twai_transmit(&e, pdMS_TO_TICKS(2));
  echoLatUs = micros() - t0;
  
  if (err == ESP_OK) {
    txOk++; 
    echoCount++;
    lastInjectedHo = setHo ? 1 : 0;
    uint16_t raw = ((b2 & 0x0F) << 8) | b3;
    lastInjectedNm = raw * 0.01f - 20.5f;
    
    // Serial.printf("[TX] id=0x%03X data=%02X%02X%02X%02X%02X%02X%02X%02X t=%.2fNm\n",
    //  e.identifier, e.data[0], e.data[1], e.data[2], e.data[3],
    //  e.data[4], e.data[5], e.data[6], e.data[7], lastInjectedNm);
  } else {
    txFail++;
    unsigned long now = millis();
    if (now - lastTxFailLog >= 2000) {
      lastTxFailLog = now;
      Serial.printf("[TX FAIL] %s total=%lu\n",
                    esp_err_to_name(err),
                    (unsigned long)txFail);
    }
  }
}

static void updateApState(const twai_message_t& f) {
  if (f.data_length_code < 8) return;
  
  uint8_t apb, apsh, apmask, hob, hosh, homask;
  portENTER_CRITICAL(&cfgMux);
  apb = cfg.apStateByte; apsh = cfg.apStateShift; apmask = cfg.apStateMask;
  hob = cfg.handsOnByte; hosh = cfg.handsOnShift; homask = cfg.handsOnMask;
  portEXIT_CRITICAL(&cfgMux);
  
  if (apb >= f.data_length_code || hob >= f.data_length_code) return;
  
  uint8_t ap = (f.data[apb] >> apsh) & apmask;
  uint8_t ho = (f.data[hob] >> hosh) & homask;
  unsigned long now = millis();
  
  portENTER_CRITICAL(&ctxMux);
  ctx.apState = ap;
  ctx.lastApStateMs = now;
  if (ho != ctx.handsOnState) {
    ctx.prevHandsOnState = ctx.handsOnState;
    ctx.handsOnState = ho;
  }
  portEXIT_CRITICAL(&ctxMux);
}

static void updateSteering(const twai_message_t& f) {
  if (f.data_length_code < 8) return;
  
  uint8_t bh, bl; 
  float scale, offs;
  portENTER_CRITICAL(&cfgMux);
  bh = cfg.steeringByteHi; 
  bl = cfg.steeringByteLo;
  scale = cfg.steeringScale; 
  offs = cfg.steeringOffset;
  portEXIT_CRITICAL(&cfgMux);
  
  if (bh >= f.data_length_code || bl >= f.data_length_code) return;
  
  int16_t raw = (int16_t)(((uint16_t)f.data[bh] << 8) | f.data[bl]);
  float deg = raw * scale + offs;
  unsigned long now = millis();
  
  portENTER_CRITICAL(&ctxMux);
  ctx.steeringAngleDeg = deg;
  ctx.lastSteeringMs = now;
  portEXIT_CRITICAL(&ctxMux);
}

static void canTask(void* arg) {
  for (;;) {
    twai_message_t f;
    
    // #4: Heartbeat counter
    canBeat++;
    
    // #2: Changed from reboot to log only every 5 seconds
    if ((millis() - bootTime) > 20000 && canAnyFrames == 0) {
      if (millis() - lastNoCanWarn > 5000) {
        Serial.println("No CAN frames yet, staying alive.");
        lastNoCanWarn = millis();
      }
    }
    
    while (twai_receive(&f, pdMS_TO_TICKS(2)) == ESP_OK) {
      canAnyFrames++;
      canRxBeat++;
      lastCanFrameMs = millis();

      unsigned long now = millis();
      if (now - lastCanLogMs >= 10000) {
        Serial.printf("[CAN] total=%lu last_id=0x%03lX dlc=%u\n",
                      (unsigned long)canAnyFrames,
                      (unsigned long)f.identifier,
                      (unsigned)f.data_length_code);
        lastCanLogMs = now;
      }

      uint16_t targetId, apStateId, steeringId;
      bool en;
      portENTER_CRITICAL(&cfgMux);
      targetId   = cfg.targetId;
      apStateId  = cfg.apStateId;
      steeringId = cfg.steeringId;
      en         = cfg.enabled;
      portEXIT_CRITICAL(&cfgMux);

      if (f.identifier == apStateId)  updateApState(f);
      if (f.identifier == steeringId) updateSteering(f);

      if (f.identifier != targetId) continue;
      rxFrames++;

      if (f.data_length_code < 5) continue;
      
      uint8_t ho = (f.data[4] >> 6) & 0x03;
      uint16_t tRaw = ((f.data[2] & 0x0F) << 8) | f.data[3];
      realHo     = ho;
      realTorque = tRaw * 0.01f - 20.5f;

      bool isOurs = false;
      if (ho == 1) {
        portENTER_CRITICAL(&cfgMux);
        for (uint8_t i = 0; i < cfg.torqueCount; i++) {
          uint16_t cfgRaw = ((cfg.torqueB2[i] & 0x0F) << 8) | cfg.torqueB3[i];
          if (tRaw == cfgRaw) { 
            isOurs = true; 
            break; 
          }
        }
        portEXIT_CRITICAL(&cfgMux);
      }

      // #8: Must see CAN before injection (>1000 frames AND delay passed)
      bool bootDelayPassed = (millis() - canInitTime) >= INJECTION_DELAY_MS;
      bool canSeen = canAnyFrames > 1000;
      
      if (en && bootDelayPassed && canSeen && !isOurs && ho <= 1) {
        echoModified(f);
      }
    }

    twai_status_info_t st;
    if (twai_get_status_info(&st) == ESP_OK) {
      unsigned long nowStatus = millis();
      if (nowStatus - lastStatusLog >= 5000) {
        Serial.printf("[TWAI] state=%d tx_err=%u rx_err=%u missed=%u txOk=%u\n",
          st.state, st.tx_error_counter, st.rx_error_counter, 
          st.rx_missed_count, (unsigned)txOk);
        lastStatusLog = nowStatus;
      }

      if (st.state == TWAI_STATE_BUS_OFF) {
        Serial.println("TWAI: Bus off, recovering...");
        twai_initiate_recovery();
        vTaskDelay(pdMS_TO_TICKS(300));
      }
    }

    vTaskDelay(1);
  }
}

extern const char INDEX_HTML[] PROGMEM;
static WebServer server(80);

static String cfgToJson() {
  Config c;
  portENTER_CRITICAL(&cfgMux); 
  c = cfg; 
  portEXIT_CRITICAL(&cfgMux);
  
  String s;
  s.reserve(512);
  
  s = "{";
  s += "\"enabled\":";    s += (c.enabled ? "true" : "false");
  s += ",\"mode\":";      s += String(c.mode);
  s += ",\"targetId\":";  s += String(c.targetId);
  s += ",\"hoRatePct\":"; s += String(c.hoRatePct);
  s += ",\"burstMs\":";   s += String(c.burstMs);
  s += ",\"pauseMs\":";   s += String(c.pauseMs);
  s += ",\"apStateId\":"; s += String(c.apStateId);
  s += ",\"steeringId\":";s += String(c.steeringId);
  s += ",\"torque\":[";
  for (uint8_t i = 0; i < c.torqueCount; i++) {
    if (i) s += ",";
    s += "{\"b2\":"; s += String(c.torqueB2[i]);
    s += ",\"b3\":"; s += String(c.torqueB3[i]);
    uint16_t raw = ((c.torqueB2[i] & 0x0F) << 8) | c.torqueB3[i];
    float nm = raw * 0.01f - 20.5f;
    s += ",\"nm\":"; s += String(nm, 2);
    s += "}";
  }
  s += "]}";
  return s;
}

static String statsToJson() {
  Context c;
  portENTER_CRITICAL(&ctxMux); 
  c = ctx; 
  portEXIT_CRITICAL(&ctxMux);
  
  String s;
  s.reserve(512);
  
  s = "{";
  s += "\"rx\":";            s += String(rxFrames);
  s += ",\"echo\":";         s += String(echoCount);
  s += ",\"txOk\":";         s += String(txOk);
  s += ",\"txFail\":";       s += String(txFail);
  s += ",\"latUs\":";        s += String(echoLatUs);
  s += ",\"ho\":";           s += String(realHo);
  s += ",\"torque\":";       s += String(realTorque, 2);
  s += ",\"injHo\":";        s += String(lastInjectedHo);
  s += ",\"injNm\":";        s += String(lastInjectedNm, 2);
  s += ",\"uptimeS\":";      s += String((millis() - bootTime) / 1000);
  s += ",\"apState\":";      s += String(c.apState);
  s += ",\"handsOnState\":"; s += String(c.handsOnState);
  s += ",\"steeringDeg\":";  s += String(c.steeringAngleDeg, 1);
  unsigned long now = millis();
  s += ",\"apStaleMs\":";    s += String((c.lastApStateMs == 0) ? 999999 : (now - c.lastApStateMs));
  s += ",\"stStaleMs\":";    s += String((c.lastSteeringMs == 0) ? 999999 : (now - c.lastSteeringMs));
  s += ",\"canAny\":";       s += String(canAnyFrames);
  s += ",\"canAgeMs\":";     s += String((lastCanFrameMs == 0) ? 999999 : (now - lastCanFrameMs));
  s += ",\"canBeat\":";      s += String(canBeat);
  s += ",\"canRxBeat\":";    s += String(canRxBeat);
  s += ",\"webBeat\":";      s += String(webBeat);
  s += ",\"twaiReady\":";    s += (twaiReady ? "true" : "false");
  
  twai_status_info_t st; 
  if (twaiReady && twai_get_status_info(&st) == ESP_OK) {
    s += ",\"canState\":";     s += String((int)st.state);
  }
  s += "}";
  return s;
}

static void httpRoot()   { server.send_P(200, "text/html", INDEX_HTML); }
static void httpConfig() { server.send(200, "application/json", cfgToJson()); }
static void httpStats()  { server.send(200, "application/json", statsToJson()); }

static void httpSetMode() {
  int m = server.arg("m").toInt();
  Config nc;
  if (m == 3) cfgDefaultsModeC(nc);
  else if (m == 1) cfgDefaultsModeB(nc);
  else        cfgDefaultsModeA(nc);
  
  portENTER_CRITICAL(&cfgMux); 
  cfg = nc; 
  portEXIT_CRITICAL(&cfgMux);
  
  cfgSave();
  server.send(200, "application/json", cfgToJson());
}

static void httpUpdate() {
  Config nc;
  portENTER_CRITICAL(&cfgMux); 
  nc = cfg; 
  portEXIT_CRITICAL(&cfgMux);
  
  if (server.hasArg("enabled"))    
    nc.enabled = (server.arg("enabled") == "1");
    
  if (server.hasArg("targetId")) {
    char* endptr;
    long val = strtol(server.arg("targetId").c_str(), &endptr, 0);
    if (*endptr == '\0' && val > 0 && val <= 0x7FF) 
      nc.targetId = (uint16_t)val;
  }
  
  if (server.hasArg("hoRatePct")) {
    int val = server.arg("hoRatePct").toInt();
    if (val >= 0 && val <= 100) nc.hoRatePct = (uint8_t)val;
  }
  
  if (server.hasArg("burstMs")) {
    int val = server.arg("burstMs").toInt();
    if (val >= 50 && val <= 10000) nc.burstMs = (uint16_t)val;
  }
  
  if (server.hasArg("pauseMs")) {
    int val = server.arg("pauseMs").toInt();
    if (val >= 0 && val <= 10000) nc.pauseMs = (uint16_t)val;
  }
  
  if (server.hasArg("apStateId")) {
    char* endptr;
    long val = strtol(server.arg("apStateId").c_str(), &endptr, 0);
    if (*endptr == '\0' && val > 0 && val <= 0x7FF) 
      nc.apStateId = (uint16_t)val;
  }
  
  if (server.hasArg("steeringId")) {
    char* endptr;
    long val = strtol(server.arg("steeringId").c_str(), &endptr, 0);
    if (*endptr == '\0' && val > 0 && val <= 0x7FF) 
      nc.steeringId = (uint16_t)val;
  }
  
  if (server.hasArg("count")) {
    uint8_t n = (uint8_t)server.arg("count").toInt();
    if (n > MAX_TORQUE_ENTRIES) n = MAX_TORQUE_ENTRIES;
    if (n < 1) n = 1;
    for (uint8_t i = 0; i < n; i++) {
      String k2 = "b2_" + String(i);
      String k3 = "b3_" + String(i);
      if (server.hasArg(k2)) {
        char* endptr;
        long val = strtol(server.arg(k2).c_str(), &endptr, 0);
        if (*endptr == '\0' && val >= 0 && val <= 255)
          nc.torqueB2[i] = (uint8_t)val;
      }
      if (server.hasArg(k3)) {
        char* endptr;
        long val = strtol(server.arg(k3).c_str(), &endptr, 0);
        if (*endptr == '\0' && val >= 0 && val <= 255)
          nc.torqueB3[i] = (uint8_t)val;
      }
    }
    nc.torqueCount = n;
  }
  
  cfgClampAll(nc);
  portENTER_CRITICAL(&cfgMux); 
  cfg = nc; 
  portEXIT_CRITICAL(&cfgMux);
  cfgSave();
  server.send(200, "application/json", cfgToJson());
}

static void httpReset() {
  Config nc; 
  cfgDefaultsModeA(nc);
  portENTER_CRITICAL(&cfgMux); 
  cfg = nc; 
  portEXIT_CRITICAL(&cfgMux);
  cfgSave();
  rxFrames = echoCount = txOk = txFail = 0;
  server.send(200, "application/json", cfgToJson());
}

static void webTask(void* arg) {
  Serial.println("WiFi: Starting AP...");
  
  WiFi.disconnect(true);
  delay(100);
  WiFi.mode(WIFI_AP);
  delay(100);
  
  uint8_t mac[6]; 
  WiFi.softAPmacAddress(mac);
  char ssid[24];
  snprintf(ssid, sizeof(ssid), "Setup-%02X%02X", mac[4], mac[5]);
  
  // #6: Retry AP startup instead of dying
  while (!WiFi.softAP(ssid, "12345678")) {
    Serial.println("WiFi: Failed to start AP, retrying...");
    vTaskDelay(pdMS_TO_TICKS(3000));
  }
  
  IPAddress ip = WiFi.softAPIP();
  Serial.printf("AP: SSID=%s IP=%s\n", ssid, ip.toString().c_str());

  server.on("/",           HTTP_GET,  httpRoot);
  server.on("/api/config", HTTP_GET,  httpConfig);
  server.on("/api/stats",  HTTP_GET,  httpStats);
  server.on("/api/mode",   HTTP_POST, httpSetMode);
  server.on("/api/update", HTTP_POST, httpUpdate);
  server.on("/api/reset",  HTTP_POST, httpReset);

  ElegantOTA.onStart([]() {
    Serial.println("OTA update process started.");
  });
  ElegantOTA.onProgress([](size_t current, size_t final) {
    Serial.printf("Progress: %u%%\n", (current * 100) / final);
  });

  ElegantOTA.onEnd([](bool success) {
    if (success) {
      Serial.println("OTA update completed successfully.");
    } else {
      Serial.println("OTA update failed.");
    }
  });
  ElegantOTA.begin(&server);             // Adds OTA server, must be before server.begin().  OTA is available at http://<myipaddress>/update
  ElegantOTA.setAuth("update", "we9v");  // Adds authentication, username update, password we9v, must be after .begin

  // Redirect /ota and /upload to /update (ElegantOTA page)
  server.on("/ota", HTTP_GET, []() {
      server.sendHeader("Location", "/update");
      server.send(302, "text/plain", "");
  });

  server.on("/upload", HTTP_GET, []() {
      server.sendHeader("Location", "/update");
      server.send(302, "text/plain", "");
  });


  server.begin();

  for (;;) {
    server.handleClient();
    ElegantOTA.loop();
    webBeat++;  // #4: Heartbeat
    vTaskDelay(1);
  }
}

void setup() {
  bootTime = millis();
  Serial.begin(115200);
  delay(1500);
  
  // #5: RTC boot count
  rtcBootCount++;
  
  esp_reset_reason_t reset_reason = esp_reset_reason();
  Serial.printf("\n=== BOOT START ===\n");
  Serial.printf("Reset reason: %d (%s)\n", reset_reason, resetReasonName(reset_reason));
  Serial.printf("RTC boot count: %lu\n", (unsigned long)rtcBootCount);
  if (reset_reason == ESP_RST_BROWNOUT) {
    Serial.println("WARNING: Brownout detected!");
  }
  
  Serial.printf("IDF version: %s\n", esp_get_idf_version());

  Serial.println("Loading config...");
  cfgLoad();
  cfgClampAll(cfg);

  Serial.printf("mode=%u id=0x%03X torqueCount=%u enabled=%u\n",
    cfg.mode, cfg.targetId, cfg.torqueCount, cfg.enabled);

  // Start dashboard first so the ESP is visible during the driver-wake delay.
  Serial.println("Creating web task...");
  BaseType_t ret2 = xTaskCreatePinnedToCore(webTask, "web", 8192, nullptr, 1, nullptr, 0);
  if (ret2 != pdPASS) {
    Serial.printf("Web task creation failed: %d\n", ret2);
    delay(3000);
    ESP.restart();
  }

  // #1: Driver-wake delay before touching CAN/TWAI.
  Serial.println("Driver-wake power detected. Waiting 10 seconds before CAN init...");
  delay(DRIVER_WAKE_DELAY_MS);

  Serial.println("Initializing TWAI...");
  twai_general_config_t g = TWAI_GENERAL_CONFIG_DEFAULT(
      (gpio_num_t)CAN_TX_PIN, (gpio_num_t)CAN_RX_PIN, TWAI_MODE_NORMAL);
  g.rx_queue_len = 256;
  g.tx_queue_len = 16;
  twai_timing_config_t t = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t f = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  esp_err_t err1 = twai_driver_install(&g, &t, &f);
  esp_err_t err2 = twai_start();
  Serial.printf("TWAI: %s / %s\n", esp_err_to_name(err1), esp_err_to_name(err2));
    
  if (err1 != ESP_OK || err2 != ESP_OK) {
    Serial.println("TWAI init failed! Rebooting...");
    delay(3000);
    ESP.restart();
  }

  // Record when CAN actually started (for injection delay calculation).
  canInitTime = millis();
  twaiReady = true;
  delay(100);

  Serial.println("Creating CAN task...");
  BaseType_t ret1 = xTaskCreatePinnedToCore(canTask, "can", 8192, nullptr, 5, nullptr, 1);
  
  // #3: Reboot instead of freeze on task creation failure.
  if (ret1 != pdPASS) {
    Serial.printf("CAN task creation failed: %d\n", ret1);
    delay(3000);
    ESP.restart();
  }
  
  Serial.println("BOOT OK");
}

// #4: Heartbeat logging in loop()
void loop() {
  static unsigned long lastBeatLog = 0;
  static uint32_t loopBeat = 0;

  loopBeat++;
  unsigned long now = millis();

  if (now - lastBeatLog >= 5000) {
    lastBeatLog = now;
    unsigned long canAgeMs = (lastCanFrameMs == 0) ? 999999 : (now - lastCanFrameMs);
    Serial.printf(
      "[BEAT] uptime=%lu loop=%lu canBeat=%lu canRxBeat=%lu webBeat=%lu canFrames=%lu canAgeMs=%lu txOk=%lu txFail=%lu heap=%u\n",
      now / 1000,
      (unsigned long)loopBeat,
      (unsigned long)canBeat,
      (unsigned long)canRxBeat,
      (unsigned long)webBeat,
      (unsigned long)canAnyFrames,
      canAgeMs,
      (unsigned long)txOk,
      (unsigned long)txFail,
      ESP.getFreeHeap()
    );
  }

  vTaskDelay(pdMS_TO_TICKS(1000));
}
