// T2CAN Unified - Dual CAN (MCP2515 + TWAI) for LilyGo T-2CAN
// CAN A (MCP2515) -> Nag Echo  (ID 0x370)
// CAN B (TWAI)    -> Summon Unlock (IDs 280, 390, 921, 1016, 1021)

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <nvs_flash.h>
#include <esp_system.h>
#include "driver/twai.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "index_html.h"

// T-2CAN board specific
#include "pin_config.h"
#include <mcp2515.h>
#include <SPI.h>

// ═══════════════════════════════════════════════════════════════
// FORWARD DECLARATIONS (needed for Arduino auto-prototypes)
// ═══════════════════════════════════════════════════════════════

struct NagConfig;
struct NagContext;

// ═══════════════════════════════════════════════════════════════
// SHARED GLOBALS
// ═══════════════════════════════════════════════════════════════

static unsigned long bootTime = 0;
static unsigned long canInitTime = 0;
static volatile bool twaiReady = false;
static volatile bool mcpReady = false;
static volatile uint32_t canAnyFrames = 0;
static volatile unsigned long lastCanFrameMs = 0;
static volatile uint32_t canBeat = 0;
static volatile uint32_t canRxBeat = 0;
static volatile uint32_t webBeat = 0;
RTC_DATA_ATTR uint32_t rtcBootCount = 0;
static Preferences prefs;

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

// ═══════════════════════════════════════════════════════════════
// MCP2515 GLOBALS
// ═══════════════════════════════════════════════════════════════

static MCP2515 Can_A(MCP2515_CS, 10000000, &SPI);
static volatile uint8_t  mcpState = 0;      // 0=OK, 1=WARN, 2=BUS-OFF
static volatile uint32_t mcpTxOk = 0;
static volatile uint32_t mcpTxFail = 0;
static volatile uint8_t  mcpTxFailConsecutive = 0;
static volatile uint32_t mcpRxCount = 0;
static unsigned long lastMcpStatusMs = 0;
static unsigned long lastMcpRecoverMs = 0;

// ═══════════════════════════════════════════════════════════════
// NAG ECHO (CAN A - MCP2515)
// ═══════════════════════════════════════════════════════════════

static const uint16_t NAG_TORQUE_RAW_MAX = 0x8B6;
static const uint16_t NAG_TORQUE_RAW_MIN = 0x74E;
static const float    NAG_TORQUE_NM_MAX  = +1.80f;
static const float    NAG_TORQUE_NM_MIN  = -1.80f;
static const uint8_t  NAG_MAX_TORQUE_ENTRIES = 8;
static const unsigned long NAG_DRIVER_WAKE_DELAY_MS = 10000;
static const unsigned long NAG_INJECTION_DELAY_MS = 15000;

enum NagMode : uint8_t { MODE_A = 0, MODE_B = 1, MODE_C = 2, MODE_CUSTOM = 3 };

struct NagConfig {
  bool     enabled;
  uint8_t  mode;
  uint16_t targetId;
  uint8_t  torqueCount;
  uint8_t  torqueB2[NAG_MAX_TORQUE_ENTRIES];
  uint8_t  torqueB3[NAG_MAX_TORQUE_ENTRIES];
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

static NagConfig nagCfg;
static portMUX_TYPE nagCfgMux = portMUX_INITIALIZER_UNLOCKED;

struct NagContext {
  uint8_t  apState;
  uint8_t  handsOnState;
  uint8_t  prevHandsOnState;
  float    steeringAngleDeg;
  unsigned long lastApStateMs;
  unsigned long lastSteeringMs;
  unsigned long state2EnterMs;
  unsigned long state3EnterMs;
  uint16_t walkSeed;
  float    lastModeCTorqueNm;
};

static NagContext nagCtx;
static portMUX_TYPE nagCtxMux = portMUX_INITIALIZER_UNLOCKED;

static volatile uint32_t nagRxFrames    = 0;
static volatile uint32_t nagEchoCount   = 0;
static volatile uint32_t nagTxOk        = 0;
static volatile uint32_t nagTxFail      = 0;
static volatile uint32_t nagEchoLatUs   = 0;
static volatile uint8_t  nagRealHo      = 0;
static volatile float    nagRealTorque  = 0;
static volatile uint8_t  nagLastInjectedHo = 0;
static volatile float    nagLastInjectedNm = 0;
static unsigned long nagLastTxFailLog = 0;

// ── Nag helpers ──

static void nagClampTorque(uint8_t& b2, uint8_t& b3) {
  uint16_t raw = ((b2 & 0x0F) << 8) | b3;
  if (raw > NAG_TORQUE_RAW_MAX) raw = NAG_TORQUE_RAW_MAX;
  if (raw < NAG_TORQUE_RAW_MIN) raw = NAG_TORQUE_RAW_MIN;
  b2 = (b2 & 0xF0) | ((raw >> 8) & 0x0F);
  b3 = raw & 0xFF;
}

static void nagNmToBytes(float nm, uint8_t& b2lo, uint8_t& b3) {
  if (nm > NAG_TORQUE_NM_MAX) nm = NAG_TORQUE_NM_MAX;
  if (nm < NAG_TORQUE_NM_MIN) nm = NAG_TORQUE_NM_MIN;
  uint16_t raw = (uint16_t)((nm + 20.5f) * 100.0f + 0.5f);
  if (raw > NAG_TORQUE_RAW_MAX) raw = NAG_TORQUE_RAW_MAX;
  if (raw < NAG_TORQUE_RAW_MIN) raw = NAG_TORQUE_RAW_MIN;
  b2lo = (raw >> 8) & 0x0F;
  b3   = raw & 0xFF;
}

static void nagCfgSetCommonDefaults(NagConfig& c) {
  c.enabled        = true;
  c.burstMs        = 1000;
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

static void nagCfgDefaultsModeA(NagConfig& c) {
  nagCfgSetCommonDefaults(c);
  c.mode        = MODE_A;
  c.targetId    = 0x370;
  c.torqueCount = 1;
  c.torqueB2[0] = 0x08;
  c.torqueB3[0] = 0xB6;
  c.hoRatePct   = 100;
}
static void nagCfgDefaultsModeB(NagConfig& c) {
  nagCfgSetCommonDefaults(c);
  c.mode        = MODE_B;
  c.targetId    = 0x370;
  c.torqueCount = 4;
  c.torqueB2[0] = 0x08; c.torqueB3[0] = 0xB6;
  c.torqueB2[1] = 0x08; c.torqueB3[1] = 0x98;
  c.torqueB2[2] = 0x07; c.torqueB3[2] = 0x6C;
  c.torqueB2[3] = 0x07; c.torqueB3[3] = 0x4E;
  c.hoRatePct   = 100;
}
static void nagCfgDefaultsModeC(NagConfig& c) {
  nagCfgSetCommonDefaults(c);
  c.mode        = MODE_C;
  c.targetId    = 0x370;
  c.torqueCount = 1;
  c.torqueB2[0] = 0x08;
  c.torqueB3[0] = 0xB6;
  c.hoRatePct   = 100;
}

static void nagCfgClampAll(NagConfig& c) {
  if (c.torqueCount < 1) c.torqueCount = 1;
  if (c.torqueCount > NAG_MAX_TORQUE_ENTRIES) c.torqueCount = NAG_MAX_TORQUE_ENTRIES;
  if (c.hoRatePct > 100) c.hoRatePct = 100;
  if (c.burstMs < 50)    c.burstMs   = 50;
  if (c.burstMs > 10000) c.burstMs   = 10000;
  if (c.pauseMs > 10000) c.pauseMs   = 10000;
  for (uint8_t i = 0; i < c.torqueCount; i++) nagClampTorque(c.torqueB2[i], c.torqueB3[i]);
}

static void nagCfgLoad() {
  Serial.println("NVS: Loading nag config...");
  if (!prefs.begin("nag", true)) {
    Serial.println("NVS: No existing nag config, using defaults");
    nagCfgDefaultsModeA(nagCfg);
    return;
  }
  if (!prefs.isKey("v")) {
    prefs.end();
    nagCfgDefaultsModeA(nagCfg);
    return;
  }
  nagCfgSetCommonDefaults(nagCfg);
  nagCfg.enabled        = prefs.getBool("en", true);
  nagCfg.mode           = prefs.getUChar("mode", 0);
  nagCfg.targetId       = prefs.getUShort("id", 0x370);
  nagCfg.torqueCount    = prefs.getUChar("tc", 1);
  size_t n = prefs.getBytes("tb2", nagCfg.torqueB2, NAG_MAX_TORQUE_ENTRIES);
  if (n == 0) { nagCfg.torqueB2[0] = 0x08; }
  n = prefs.getBytes("tb3", nagCfg.torqueB3, NAG_MAX_TORQUE_ENTRIES);
  if (n == 0) { nagCfg.torqueB3[0] = 0xB6; }
  nagCfg.hoRatePct      = prefs.getUChar("ho", 100);
  nagCfg.burstMs        = prefs.getUShort("bms", 1000);
  nagCfg.pauseMs        = prefs.getUShort("pms", 1500);
  nagCfg.apStateId      = prefs.getUShort("apid", 0x399);
  nagCfg.steeringId     = prefs.getUShort("stid", 0x129);
  prefs.end();
  nagCfgClampAll(nagCfg);
  Serial.println("NVS: Nag config loaded OK");
}

static void nagCfgSave() {
  nagCfgClampAll(nagCfg);
  if (!prefs.begin("nag", false)) {
    Serial.println("NVS: Nag save failed - could not open");
    return;
  }
  prefs.putBool("en",     nagCfg.enabled);
  prefs.putUChar("mode",  nagCfg.mode);
  prefs.putUShort("id",   nagCfg.targetId);
  prefs.putUChar("tc",    nagCfg.torqueCount);
  prefs.putBytes("tb2",   nagCfg.torqueB2, NAG_MAX_TORQUE_ENTRIES);
  prefs.putBytes("tb3",   nagCfg.torqueB3, NAG_MAX_TORQUE_ENTRIES);
  prefs.putUChar("ho",    nagCfg.hoRatePct);
  prefs.putUShort("bms",  nagCfg.burstMs);
  prefs.putUShort("pms",  nagCfg.pauseMs);
  prefs.putUShort("apid", nagCfg.apStateId);
  prefs.putUShort("stid", nagCfg.steeringId);
  prefs.putUChar("v",     2);
  prefs.end();
}

// ── Nag decide injection (raw data version) ──

static bool nagDecideInjection(uint8_t dlc,
                            uint8_t& out_b2, uint8_t& out_b3, bool& out_setHo) {
  if (dlc < 8) return false;
  unsigned long now = millis();

  uint8_t  mode, tCount, hoPct;
  uint16_t burstMs, pauseMs;
  uint8_t  tB2[NAG_MAX_TORQUE_ENTRIES], tB3[NAG_MAX_TORQUE_ENTRIES];

  portENTER_CRITICAL(&nagCfgMux);
  mode    = nagCfg.mode;
  tCount  = nagCfg.torqueCount;
  hoPct   = nagCfg.hoRatePct;
  burstMs = nagCfg.burstMs;
  pauseMs = nagCfg.pauseMs;
  for (uint8_t i = 0; i < tCount; i++) {
    tB2[i] = nagCfg.torqueB2[i];
    tB3[i] = nagCfg.torqueB3[i];
  }
  portEXIT_CRITICAL(&nagCfgMux);

  static uint8_t  tIdx = 0;
  static uint16_t hoSeq = 0;
  static uint32_t lastChangeMs = 0;
  static uint8_t  prevMode = 0xFF;

  if (mode != prevMode) {
    tIdx = 0; hoSeq = 0; lastChangeMs = now; prevMode = mode;
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
    if (now - lastChangeMs >= 200) { tIdx = (tIdx + 1) % tCount; lastChangeMs = now; }
    out_b2 = tB2[tIdx];
    out_b3 = tB3[tIdx];
    out_setHo = true;
    return true;
  }

  if (mode == MODE_C) {
    NagContext c;
    portENTER_CRITICAL(&nagCtxMux); c = nagCtx; portEXIT_CRITICAL(&nagCtxMux);

    const unsigned long FRESH_MS = 1000;
    if (now - c.lastApStateMs  > FRESH_MS) return false;
    if (now - c.lastSteeringMs > FRESH_MS) return false;
    if (c.apState < 3 || c.apState > 6)   return false;
    if (fabsf(c.steeringAngleDeg) > 5.0f) return false;

    float torqueNm; bool setHo;
    if (c.handsOnState == 1) return false;
    else if (c.handsOnState == 2) {
      if (c.state2EnterMs == 0) return false;
      if (now - c.state2EnterMs < 2000) return false;
      uint16_t s = c.walkSeed;
      s = (uint16_t)(s * 1103u + 12345u);
      float delta = ((int)(s & 0x1F) - 16) * 0.05f;
      float prev = c.lastModeCTorqueNm;
      float mag = fabsf(prev) + delta;
      if (mag < 0.5f) mag = 0.5f;
      if (mag > 1.8f) mag = 1.8f;
      torqueNm = (c.steeringAngleDeg > 0.0f) ? -mag : +mag;
      setHo = (fabsf(torqueNm) >= 1.0f);
      portENTER_CRITICAL(&nagCtxMux);
      nagCtx.walkSeed = s;
      nagCtx.lastModeCTorqueNm = torqueNm;
      portEXIT_CRITICAL(&nagCtxMux);
    }
    else if (c.handsOnState == 3) {
      if (c.state3EnterMs == 0) return false;
      if (now - c.state3EnterMs < 1000) return false;
      uint32_t activeMs = (uint32_t)(now - c.state3EnterMs - 1000);
      uint32_t phase = activeMs % 1000;
      if (phase < 500) torqueNm = -1.8f + (phase / 500.0f) * 3.6f;
      else             torqueNm = +1.8f - ((phase - 500) / 500.0f) * 3.6f;
      setHo = (fabsf(torqueNm) >= 1.0f);
    }
    else return false;

    nagNmToBytes(torqueNm, out_b2, out_b3);
    out_setHo = setHo;
    return true;
  }
  return false;
}

// ── Nag context updates (raw data) ──

static void nagUpdateApState(const uint8_t* data, uint8_t dlc) {
  if (dlc < 8) return;
  uint8_t apb, apsh, apmask, hob, hosh, homask;
  portENTER_CRITICAL(&nagCfgMux);
  apb = nagCfg.apStateByte; apsh = nagCfg.apStateShift; apmask = nagCfg.apStateMask;
  hob = nagCfg.handsOnByte; hosh = nagCfg.handsOnShift; homask = nagCfg.handsOnMask;
  portEXIT_CRITICAL(&nagCfgMux);
  if (apb >= dlc || hob >= dlc) return;
  uint8_t ap = (data[apb] >> apsh) & apmask;
  uint8_t ho = (data[hob] >> hosh) & homask;
  unsigned long now = millis();
  portENTER_CRITICAL(&nagCtxMux);
  nagCtx.apState = ap;
  nagCtx.lastApStateMs = now;
  if (ho != nagCtx.handsOnState) {
    nagCtx.prevHandsOnState = nagCtx.handsOnState;
    nagCtx.handsOnState = ho;
    if (ho == 2 && nagCtx.state2EnterMs == 0) nagCtx.state2EnterMs = now;
    if (ho != 2) nagCtx.state2EnterMs = 0;
    if (ho == 3 && nagCtx.state3EnterMs == 0) nagCtx.state3EnterMs = now;
    if (ho != 3) nagCtx.state3EnterMs = 0;
  }
  portEXIT_CRITICAL(&nagCtxMux);
}

static void nagUpdateSteering(const uint8_t* data, uint8_t dlc) {
  if (dlc < 8) return;
  uint8_t bh, bl; float scale, offs;
  portENTER_CRITICAL(&nagCfgMux);
  bh = nagCfg.steeringByteHi; bl = nagCfg.steeringByteLo;
  scale = nagCfg.steeringScale; offs = nagCfg.steeringOffset;
  portEXIT_CRITICAL(&nagCfgMux);
  if (bh >= dlc || bl >= dlc) return;
  int16_t raw = (int16_t)(((uint16_t)data[bh] << 8) | data[bl]);
  float deg = raw * scale + offs;
  unsigned long now = millis();
  portENTER_CRITICAL(&nagCtxMux);
  nagCtx.steeringAngleDeg = deg;
  nagCtx.lastSteeringMs = now;
  portEXIT_CRITICAL(&nagCtxMux);
}

// ── Nag process frame from MCP2515 ──

static void nagProcessMcpFrame(const struct can_frame& rxf) {
  uint16_t id = rxf.can_id & 0x7FF;
  uint8_t dlc = rxf.can_dlc;
  if (dlc < 1) return;

  uint16_t targetId, apStateId, steeringId;
  bool en;
  portENTER_CRITICAL(&nagCfgMux);
  targetId   = nagCfg.targetId;
  apStateId  = nagCfg.apStateId;
  steeringId = nagCfg.steeringId;
  en         = nagCfg.enabled;
  portEXIT_CRITICAL(&nagCfgMux);

  if (id == apStateId)  nagUpdateApState(rxf.data, dlc);
  if (id == steeringId) nagUpdateSteering(rxf.data, dlc);

  if (id != targetId) return;
  nagRxFrames++;

  if (dlc < 5) return;
  uint8_t ho = (rxf.data[4] >> 6) & 0x03;
  uint16_t tRaw = ((rxf.data[2] & 0x0F) << 8) | rxf.data[3];
  nagRealHo     = ho;
  nagRealTorque = tRaw * 0.01f - 20.5f;

  bool isOurs = false;
  if (ho == 1) {
    portENTER_CRITICAL(&nagCfgMux);
    for (uint8_t i = 0; i < nagCfg.torqueCount; i++) {
      uint16_t cfgRaw = ((nagCfg.torqueB2[i] & 0x0F) << 8) | nagCfg.torqueB3[i];
      if (tRaw == cfgRaw) { isOurs = true; break; }
    }
    portEXIT_CRITICAL(&nagCfgMux);
  }

  bool bootDelayPassed = (millis() - canInitTime) >= NAG_INJECTION_DELAY_MS;
  bool canSeen = mcpRxCount > 1000;
  if (en && bootDelayPassed && canSeen && !isOurs && ho <= 1) {
    uint8_t b2 = 0, b3 = 0; bool setHo = false;
    if (nagDecideInjection(dlc, b2, b3, setHo)) {
      struct can_frame txf;
      txf.can_id = rxf.can_id;
      txf.can_dlc = rxf.can_dlc;
      memcpy(txf.data, rxf.data, 8);
      txf.data[2] = (txf.data[2] & 0xF0) | (b2 & 0x0F);
      txf.data[3] = b3;
      txf.data[4] = setHo ? (txf.data[4] | 0x40) : txf.data[4];
      txf.data[6] = (txf.data[6] & 0xF0) | (((txf.data[6] & 0x0F) + 1) & 0x0F);
      uint16_t s = txf.data[0] + txf.data[1] + txf.data[2] + txf.data[3]
                 + txf.data[4] + txf.data[5] + txf.data[6];
      txf.data[7] = (uint8_t)((s + 0x73) & 0xFF);

      unsigned long t0 = micros();
      MCP2515::ERROR err = Can_A.sendMessage(&txf);
      nagEchoLatUs = micros() - t0;

      if (err == MCP2515::ERROR_OK) {
        mcpTxOk++; nagEchoCount++;
        mcpTxFailConsecutive = 0;
        nagLastInjectedHo = setHo ? 1 : 0;
        uint16_t raw = ((b2 & 0x0F) << 8) | b3;
        nagLastInjectedNm = raw * 0.01f - 20.5f;
      } else {
        mcpTxFail++;
        if (mcpTxFailConsecutive < 255) mcpTxFailConsecutive++;
        unsigned long now = millis();
        if (now - nagLastTxFailLog >= 2000) {
          nagLastTxFailLog = now;
          Serial.printf("[NAG TX FAIL] MCP err=%d total=%lu\n", (int)err, (unsigned long)mcpTxFail);
        }
      }
    }
  }
}

// ═══════════════════════════════════════════════════════════════
// SUMMON UNLOCK (CAN B - TWAI)
// ═══════════════════════════════════════════════════════════════

static inline uint8_t readMuxID(const uint8_t *data) {
    return data[0] & 0x07;
}
static inline bool getBit(const uint8_t *data, int bit) {
    return (data[bit / 8] >> (bit % 8)) & 0x01;
}
static inline void setBit(uint8_t *data, int bit, bool val) {
    uint8_t mask = (uint8_t)(1U << (bit % 8));
    if (val) data[bit / 8] |=  mask;
    else     data[bit / 8] &= ~mask;
}
static inline uint8_t readDIGear(const uint8_t *data) {
    return (data[2] >> 5) & 0x07;
}
static inline uint8_t readVehicleGear(const uint8_t *data) {
    return (data[2] >> 5) & 0x07;
}
static inline int gearState(uint8_t gear) {
    if (gear == 1)             return  1;
    if (gear == 2 || gear == 3 || gear == 4) return 0;
    return -1;
}
static inline uint8_t readDASStatus(const uint8_t *data) {
    return data[0] & 0x07;
}
static inline bool isDASActive(uint8_t status) {
    return status == 2 || status == 3 || status == 4;
}

static portMUX_TYPE stateMux = portMUX_INITIALIZER_UNLOCKED;
static volatile bool summonEnabled = true;
static volatile bool gateAPActive  = false;
static volatile bool gateParked    = true;
static volatile bool gateSummoning = false;
static volatile bool sprSeen  = false;
static volatile bool lastAca  = false;
#define PARKED_TIMEOUT_MS  5000
static volatile uint32_t last280Millis = 0;

static volatile uint32_t sumRxMux1   = 0;
static volatile uint32_t sumTxOk     = 0;
static volatile uint32_t sumTxFail   = 0;
static volatile uint32_t sumRx280    = 0;
static volatile uint32_t sumRx390    = 0;
static volatile uint32_t sumRx921    = 0;
static volatile uint32_t sumRx1016   = 0;
static char gateBlockReason[48] = "boot";

static inline bool injectionGateOpen() {
    return gateParked || gateSummoning;
}

static void recomputeSummoning() {
    gateSummoning = lastAca && sprSeen;
}

static void clearSummonOnPark() {
    gateSummoning = false;
    sprSeen       = false;
}

static void clearSummonOnParkIfAcaInactive(uint8_t gear) {
    if (gear == 1 && !lastAca)
        clearSummonOnPark();
}

static void handle280(const uint8_t *data) {
    sumRx280++;
    last280Millis = (uint32_t)millis();
    uint8_t gear = readDIGear(data);
    int     gs   = gearState(gear);
    portENTER_CRITICAL(&stateMux);
    if (gs == 1)  gateParked = true;
    if (gs == 0)  gateParked = false;
    bool aca = (data[6] & 0x04) != 0;
    if (lastAca && !aca)
        sprSeen = false;
    lastAca = aca;
    recomputeSummoning();
    clearSummonOnParkIfAcaInactive(gear);
    portEXIT_CRITICAL(&stateMux);
}

static void handle390(const uint8_t *data) {
    sumRx390++;
    uint8_t gear = readVehicleGear(data);
    int     gs   = gearState(gear);
    if (gs < 0) return;
    portENTER_CRITICAL(&stateMux);
    uint32_t age = (uint32_t)millis() - last280Millis;
    if (last280Millis == 0 || age > PARKED_TIMEOUT_MS) {
        gateParked = (gs == 1);
        clearSummonOnParkIfAcaInactive(gear);
    }
    portEXIT_CRITICAL(&stateMux);
}

static void handle921(const uint8_t *data) {
    sumRx921++;
    bool ap = isDASActive(readDASStatus(data));
    portENTER_CRITICAL(&stateMux);
    gateAPActive = ap;
    portEXIT_CRITICAL(&stateMux);
}

static void handle1016(const uint8_t *data, uint8_t dlc) {
    if (dlc < 4) return;
    sumRx1016++;
    uint8_t spr = (data[3] >> 4) & 0x0F;
    portENTER_CRITICAL(&stateMux);
    if (spr != 0)
        sprSeen = true;
    recomputeSummoning();
    portEXIT_CRITICAL(&stateMux);
}

static void injectSummon(const twai_message_t &src) {
    bool en, gate;
    portENTER_CRITICAL(&stateMux);
    en   = summonEnabled;
    gate = injectionGateOpen();
    if (!gate) {
        if (!gateAPActive  && !gateParked && !gateSummoning)
            strncpy(gateBlockReason, "AP-,Park-,Summon-", sizeof(gateBlockReason));
    }
    portEXIT_CRITICAL(&stateMux);
    if (!en || !gate) return;

    twai_message_t out;
    out.identifier       = src.identifier;
    out.data_length_code = src.data_length_code;
    out.flags            = 0;
    for (int i = 0; i < 8; i++) out.data[i] = src.data[i];
    setBit(out.data, 19, false);
    setBit(out.data, 47, true);
    sumRxMux1++;
    esp_err_t err = twai_transmit(&out, pdMS_TO_TICKS(2));
    if (err == ESP_OK) sumTxOk++;
    else               sumTxFail++;
}

static void summonCfgLoad() {
    prefs.begin("summon", true);
    summonEnabled = prefs.getBool("en", true);
    prefs.end();
}

static void summonCfgSave() {
    prefs.begin("summon", false);
    prefs.putBool("en", summonEnabled);
    prefs.end();
}

// ═══════════════════════════════════════════════════════════════
// BLE GATT (Summon control)
// ═══════════════════════════════════════════════════════════════

#define BLE_SERVICE_UUID   "12345678-1234-1234-1234-123456789abc"
#define BLE_CHAR_CTRL_UUID "12345678-1234-1234-1234-123456789001"
#define BLE_CHAR_STAT_UUID "12345678-1234-1234-1234-123456789002"

static BLECharacteristic *bleStatChar = nullptr;
static volatile bool      bleConnected = false;

class BleServerCb : public BLEServerCallbacks {
    void onConnect(BLEServer *)    override {
        bleConnected = true;
        Serial.println("[BLE] Client connected");
    }
    void onDisconnect(BLEServer *s) override {
        bleConnected = false;
        Serial.println("[BLE] Client disconnected - re-advertising");
        s->startAdvertising();
    }
};

class BleCtrlCb : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *c) override {
        String val = c->getValue().c_str();
        bool next = (val == "1" || val == "true" || val == "on");
        portENTER_CRITICAL(&stateMux);
        summonEnabled = next;
        portEXIT_CRITICAL(&stateMux);
        summonCfgSave();
        Serial.printf("[BLE] summonEnabled -> %s\n", next ? "true" : "false");
    }
};

static void bleSetup() {
    BLEDevice::init("SummonUnlock");
    BLEServer *srv = BLEDevice::createServer();
    srv->setCallbacks(new BleServerCb());
    BLEService *svc = srv->createService(BLE_SERVICE_UUID);
    BLECharacteristic *ctrlChar = svc->createCharacteristic(
        BLE_CHAR_CTRL_UUID, BLECharacteristic::PROPERTY_WRITE);
    ctrlChar->setCallbacks(new BleCtrlCb());
    bleStatChar = svc->createCharacteristic(
        BLE_CHAR_STAT_UUID,
        BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ);
    bleStatChar->addDescriptor(new BLE2902());
    svc->start();
    BLEAdvertising *adv = BLEDevice::getAdvertising();
    adv->addServiceUUID(BLE_SERVICE_UUID);
    adv->setScanResponse(true);
    adv->setMinPreferred(0x06);
    BLEDevice::startAdvertising();
    Serial.println("[BLE] Advertising - SummonUnlock");
}

static String summonStatsToJson();

static void bleTask(void *arg) {
    for (;;) {
        if (bleConnected && bleStatChar) {
            String j = summonStatsToJson();
            bleStatChar->setValue(j.c_str());
            bleStatChar->notify();
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// ═══════════════════════════════════════════════════════════════
// WEB SERVER
// ═══════════════════════════════════════════════════════════════

extern const char INDEX_HTML[] PROGMEM;
static WebServer server(80);

static String nagCfgToJson() {
  NagConfig c;
  portENTER_CRITICAL(&nagCfgMux);
  c = nagCfg;
  portEXIT_CRITICAL(&nagCfgMux);
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

static String nagStatsToJson() {
  NagContext c;
  portENTER_CRITICAL(&nagCtxMux); c = nagCtx; portEXIT_CRITICAL(&nagCtxMux);
  String s;
  s.reserve(512);
  s = "{";
  s += "\"rx\":";            s += String(nagRxFrames);
  s += ",\"echo\":";         s += String(nagEchoCount);
  s += ",\"txOk\":";         s += String(mcpTxOk);
  s += ",\"txFail\":";       s += String(mcpTxFail);
  s += ",\"latUs\":";        s += String(nagEchoLatUs);
  s += ",\"ho\":";           s += String(nagRealHo);
  s += ",\"torque\":";       s += String(nagRealTorque, 2);
  s += ",\"injHo\":";        s += String(nagLastInjectedHo);
  s += ",\"injNm\":";        s += String(nagLastInjectedNm, 2);
  s += ",\"uptimeS\":";      s += String((millis() - bootTime) / 1000);
  s += ",\"apState\":";      s += String(c.apState);
  s += ",\"handsOnState\":"; s += String(c.handsOnState);
  s += ",\"steeringDeg\":";  s += String(c.steeringAngleDeg, 1);
  unsigned long now = millis();
  s += ",\"apStaleMs\":";    s += String((c.lastApStateMs == 0) ? 999999 : (now - c.lastApStateMs));
  s += ",\"stStaleMs\":";    s += String((c.lastSteeringMs == 0) ? 999999 : (now - c.lastSteeringMs));
  s += ",\"canAState\":";    s += String((int)mcpState);
  s += "}";
  return s;
}

static String summonStatsToJson() {
    bool en, ap, parked, summon, aca, spr;
    uint32_t rmx, tok, tfail, r280, r390, r921, r1016;
    portENTER_CRITICAL(&stateMux);
    en     = summonEnabled;
    ap     = gateAPActive;
    parked = gateParked;
    summon = gateSummoning;
    aca    = lastAca;
    spr    = sprSeen;
    rmx    = sumRxMux1;
    tok    = sumTxOk;
    tfail  = sumTxFail;
    r280   = sumRx280;
    r390   = sumRx390;
    r921   = sumRx921;
    r1016  = sumRx1016;
    portEXIT_CRITICAL(&stateMux);
    bool gate = parked || summon;
    twai_status_info_t st; twai_get_status_info(&st);
    String s = "{";
    s += "\"enabled\":"  + String(en     ? "true" : "false");
    s += ",\"gate\":"    + String(gate   ? "true" : "false");
    s += ",\"ap\":"      + String(ap     ? "true" : "false");
    s += ",\"parked\":"  + String(parked ? "true" : "false");
    s += ",\"summon\":"  + String(summon ? "true" : "false");
    s += ",\"aca\":"     + String(aca    ? "true" : "false");
    s += ",\"spr\":"     + String(spr    ? "true" : "false");
    s += ",\"rxMux1\":"  + String(rmx);
    s += ",\"txOk\":"    + String(tok);
    s += ",\"txFail\":"  + String(tfail);
    s += ",\"rx280\":"   + String(r280);
    s += ",\"rx390\":"   + String(r390);
    s += ",\"rx921\":"   + String(r921);
    s += ",\"rx1016\":"  + String(r1016);
    s += ",\"canState\":" + String((int)st.state);
    s += ",\"uptimeS\":"  + String((millis() - bootTime) / 1000);
    s += "}";
    return s;
}

static void httpRoot()   { server.send_P(200, "text/html", INDEX_HTML); }
static void httpNagConfig() { server.send(200, "application/json", nagCfgToJson()); }
static void httpNagStats()  { server.send(200, "application/json", nagStatsToJson()); }

static void httpNagSetMode() {
  int m = server.arg("m").toInt();
  NagConfig nc;
  if      (m == 1) nagCfgDefaultsModeB(nc);
  else if (m == 2) nagCfgDefaultsModeC(nc);
  else             nagCfgDefaultsModeA(nc);
  portENTER_CRITICAL(&nagCfgMux); nagCfg = nc; portEXIT_CRITICAL(&nagCfgMux);
  nagCfgSave();
  server.send(200, "application/json", nagCfgToJson());
}

static void httpNagUpdate() {
  NagConfig nc;
  portENTER_CRITICAL(&nagCfgMux); nc = nagCfg; portEXIT_CRITICAL(&nagCfgMux);
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
    if (n > NAG_MAX_TORQUE_ENTRIES) n = NAG_MAX_TORQUE_ENTRIES;
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
  nagCfgClampAll(nc);
  portENTER_CRITICAL(&nagCfgMux); nagCfg = nc; portEXIT_CRITICAL(&nagCfgMux);
  nagCfgSave();
  server.send(200, "application/json", nagCfgToJson());
}

static void httpNagReset() {
  NagConfig nc;
  nagCfgDefaultsModeA(nc);
  portENTER_CRITICAL(&nagCfgMux); nagCfg = nc; portEXIT_CRITICAL(&nagCfgMux);
  nagCfgSave();
  nagRxFrames = nagEchoCount = mcpTxOk = mcpTxFail = 0;
  server.send(200, "application/json", nagCfgToJson());
}

static void httpSummonStats()  { server.send(200, "application/json", summonStatsToJson()); }
static void httpSummonEnable() {
    portENTER_CRITICAL(&stateMux); summonEnabled = true;  portEXIT_CRITICAL(&stateMux);
    summonCfgSave();
    server.send(200, "application/json", summonStatsToJson());
}
static void httpSummonDisable() {
    portENTER_CRITICAL(&stateMux); summonEnabled = false; portEXIT_CRITICAL(&stateMux);
    summonCfgSave();
    server.send(200, "application/json", summonStatsToJson());
}

static void webTask(void *arg) {
  Serial.println("WiFi: Starting AP...");
  WiFi.disconnect(true);
  delay(100);
  WiFi.mode(WIFI_AP);
  delay(100);
  uint8_t mac[6];
  WiFi.softAPmacAddress(mac);
  char ssid[24];
  snprintf(ssid, sizeof(ssid), "T2CAN-%02X%02X", mac[4], mac[5]);
  while (!WiFi.softAP(ssid, "12345678")) {
    Serial.println("WiFi: Failed to start AP, retrying...");
    vTaskDelay(pdMS_TO_TICKS(3000));
  }
  IPAddress ip = WiFi.softAPIP();
  Serial.printf("AP: SSID=%s IP=%s\n", ssid, ip.toString().c_str());

  server.on("/",                  HTTP_GET,  httpRoot);
  server.on("/api/nag/config",    HTTP_GET,  httpNagConfig);
  server.on("/api/nag/stats",     HTTP_GET,  httpNagStats);
  server.on("/api/nag/mode",      HTTP_POST, httpNagSetMode);
  server.on("/api/nag/update",    HTTP_POST, httpNagUpdate);
  server.on("/api/nag/reset",     HTTP_POST, httpNagReset);
  server.on("/api/summon/stats",  HTTP_GET,  httpSummonStats);
  server.on("/api/summon/enable", HTTP_POST, httpSummonEnable);
  server.on("/api/summon/disable",HTTP_POST, httpSummonDisable);
  server.begin();

  for (;;) {
    server.handleClient();
    webBeat++;
    vTaskDelay(1);
  }
}

// ═══════════════════════════════════════════════════════════════
// CAN TASKS
// ═══════════════════════════════════════════════════════════════

static void canTaskMcp(void* arg) {
  Serial.println("[CAN A] MCP2515 task started");
  for (;;) {
    struct can_frame rxf;
    while (Can_A.readMessage(&rxf) == MCP2515::ERROR_OK) {
      mcpRxCount++;
      nagProcessMcpFrame(rxf);
    }

    // MCP2515 status check (using public API only)
    unsigned long now = millis();
    if (now - lastMcpStatusMs >= 1000) {
      lastMcpStatusMs = now;
      uint8_t consecutive = mcpTxFailConsecutive;
      if (consecutive > 5) {
        mcpState = 2; // BUS-OFF
        if (now - lastMcpRecoverMs > 3000) {
          lastMcpRecoverMs = now;
          Serial.println("[CAN A] MCP2515 bus-off detected (TX fails), resetting...");
          Can_A.reset();
          Can_A.setBitrate(CAN_500KBPS);
          Can_A.setNormalMode();
          mcpTxFailConsecutive = 0;
        }
      } else if (consecutive > 0) {
        mcpState = 1; // Warning
      } else {
        mcpState = 0; // OK
      }
    }

    vTaskDelay(1);
  }
}

static void canTaskTwai(void* arg) {
  Serial.println("[CAN B] TWAI task started");
  unsigned long lastTwaiStatusMs = 0;
  unsigned long lastNoCanWarn = 0;

  for (;;) {
    twai_message_t f;
    while (twai_receive(&f, pdMS_TO_TICKS(2)) == ESP_OK) {
      canAnyFrames++;
      canRxBeat++;
      lastCanFrameMs = millis();

      switch (f.identifier) {
        case 280:
          if (f.data_length_code >= 7) handle280(f.data);
          break;
        case 390:
          if (f.data_length_code >= 8) handle390(f.data);
          break;
        case 921:
          if (f.data_length_code >= 1) handle921(f.data);
          break;
        case 1016:
          handle1016(f.data, f.data_length_code);
          break;
        case 1021:
          if (f.data_length_code >= 8 && readMuxID(f.data) == 1)
            injectSummon(f);
          break;
        default:
          break;
      }
    }

    // TWAI status check
    unsigned long now = millis();
    if (now - lastTwaiStatusMs >= 5000) {
      lastTwaiStatusMs = now;
      twai_status_info_t st;
      if (twai_get_status_info(&st) == ESP_OK) {
        if (st.state == TWAI_STATE_BUS_OFF) {
          Serial.println("[CAN B] TWAI bus-off, recovering...");
          twai_initiate_recovery();
        }
      }
    }

    // No-CAN warning (shared counter)
    if ((millis() - bootTime) > 20000 && canAnyFrames == 0) {
      if (millis() - lastNoCanWarn > 5000) {
        Serial.println("No CAN frames yet on either bus, staying alive.");
        lastNoCanWarn = millis();
      }
    }

    // Summon watchdog: if CAN 280 silent > PARKED_TIMEOUT_MS
    uint32_t nowMs = (uint32_t)millis();
    portENTER_CRITICAL(&stateMux);
    bool can280Stale = (last280Millis > 0) && (nowMs - last280Millis > PARKED_TIMEOUT_MS);
    if (can280Stale) gateParked = true;
    portEXIT_CRITICAL(&stateMux);

    vTaskDelay(1);
  }
}

// ═══════════════════════════════════════════════════════════════
// SETUP / LOOP
// ═══════════════════════════════════════════════════════════════

void setup() {
  bootTime = millis();
  Serial.begin(115200);
  delay(1500);

  rtcBootCount++;
  esp_reset_reason_t reset_reason = esp_reset_reason();
  Serial.printf("\n=== T2CAN Unified BOOT ===\n");
  Serial.printf("Reset reason: %d (%s)\n", reset_reason, resetReasonName(reset_reason));
  Serial.printf("RTC boot count: %lu\n", (unsigned long)rtcBootCount);
  if (reset_reason == ESP_RST_BROWNOUT) {
    Serial.println("WARNING: Brownout detected!");
  }
  Serial.printf("IDF version: %s\n", esp_get_idf_version());

  // NVS init
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    Serial.println("NVS: Corrupted, erasing...");
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  if (err != ESP_OK) {
    Serial.printf("NVS: Init failed %d\n", err);
  }

  // Load configs
  nagCfgLoad();
  summonCfgLoad();

  Serial.printf("Nag mode=%u id=0x%03X torqueCount=%u enabled=%u\n",
    nagCfg.mode, nagCfg.targetId, nagCfg.torqueCount, nagCfg.enabled);
  Serial.printf("Summon enabled=%s\n", summonEnabled ? "true" : "false");

  // Start web task first (Core 0)
  BaseType_t retWeb = xTaskCreatePinnedToCore(webTask, "web", 8192, nullptr, 1, nullptr, 0);
  if (retWeb != pdPASS) {
    Serial.printf("Web task creation failed: %d\n", retWeb);
    delay(3000);
    ESP.restart();
  }

  // Driver-wake delay before CAN init
  Serial.println("Driver-wake power detected. Waiting 10 seconds before CAN init...");
  delay(NAG_DRIVER_WAKE_DELAY_MS);

  // ══ Init CAN A (MCP2515) ══
  Serial.println("[CAN A] Initializing MCP2515...");
  pinMode(MCP2515_RST, OUTPUT);
  digitalWrite(MCP2515_RST, HIGH);
  delay(100);
  digitalWrite(MCP2515_RST, LOW);
  delay(100);
  digitalWrite(MCP2515_RST, HIGH);
  delay(100);

  SPI.begin(MCP2515_SCLK, MCP2515_MISO, MCP2515_MOSI, MCP2515_CS);

  Can_A.reset();
  Can_A.setBitrate(CAN_500KBPS);
  Can_A.setNormalMode();
  mcpReady = true;
  Serial.println("[CAN A] MCP2515 ready (500 kbps)");

  // ══ Init CAN B (TWAI) ══
  Serial.println("[CAN B] Initializing TWAI...");
  twai_general_config_t g = TWAI_GENERAL_CONFIG_DEFAULT(
      (gpio_num_t)CAN_TX, (gpio_num_t)CAN_RX, TWAI_MODE_NORMAL);
  g.rx_queue_len = 256;
  g.tx_queue_len = 16;
  twai_timing_config_t t = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t f = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  esp_err_t err1 = twai_driver_install(&g, &t, &f);
  esp_err_t err2 = twai_start();
  Serial.printf("[CAN B] TWAI: %s / %s\n", esp_err_to_name(err1), esp_err_to_name(err2));

  if (err1 != ESP_OK || err2 != ESP_OK) {
    Serial.println("[CAN B] TWAI init failed! Rebooting...");
    delay(3000);
    ESP.restart();
  }

  uint32_t alerts_to_enable = TWAI_ALERT_TX_IDLE | TWAI_ALERT_TX_SUCCESS |
                              TWAI_ALERT_TX_FAILED | TWAI_ALERT_ERR_PASS |
                              TWAI_ALERT_BUS_ERROR | TWAI_ALERT_RX_DATA |
                              TWAI_ALERT_RX_QUEUE_FULL;
  if (twai_reconfigure_alerts(alerts_to_enable, NULL) == ESP_OK) {
    Serial.println("[CAN B] TWAI alerts configured");
  }

  canInitTime = millis();
  twaiReady = true;
  delay(100);

  // Start CAN tasks
  BaseType_t retMcp = xTaskCreatePinnedToCore(canTaskMcp, "canA", 8192, nullptr, 5, nullptr, 1);
  if (retMcp != pdPASS) {
    Serial.printf("CAN A task creation failed: %d\n", retMcp);
    delay(3000);
    ESP.restart();
  }

  BaseType_t retTwai = xTaskCreatePinnedToCore(canTaskTwai, "canB", 8192, nullptr, 4, nullptr, 1);
  if (retTwai != pdPASS) {
    Serial.printf("CAN B task creation failed: %d\n", retTwai);
    delay(3000);
    ESP.restart();
  }

  // BLE
  bleSetup();
  xTaskCreatePinnedToCore(bleTask, "ble", 4096, nullptr, 1, nullptr, 0);

  Serial.println("BOOT OK");
}

void loop() {
  static unsigned long lastBeatLog = 0;
  static uint32_t loopBeat = 0;
  loopBeat++;
  unsigned long now = millis();

  if (now - lastBeatLog >= 5000) {
    lastBeatLog = now;
    unsigned long canAgeMs = (lastCanFrameMs == 0) ? 999999 : (now - lastCanFrameMs);
    Serial.printf(
      "[BEAT] uptime=%lu loop=%lu canBeat=%lu canRxBeat=%lu webBeat=%lu canFrames=%lu canAgeMs=%lu mcpTxOk=%lu mcpTxFail=%lu sumTxOk=%lu sumTxFail=%lu heap=%u\n",
      now / 1000,
      (unsigned long)loopBeat,
      (unsigned long)canBeat,
      (unsigned long)canRxBeat,
      (unsigned long)webBeat,
      (unsigned long)canAnyFrames,
      canAgeMs,
      (unsigned long)mcpTxOk,
      (unsigned long)mcpTxFail,
      (unsigned long)sumTxOk,
      (unsigned long)sumTxFail,
      ESP.getFreeHeap()
    );
  }
  vTaskDelay(pdMS_TO_TICKS(1000));
}