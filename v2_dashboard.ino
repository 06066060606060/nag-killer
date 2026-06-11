#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include "driver/twai.h"

#define CAN_TX_PIN    5
#define CAN_RX_PIN    6

// ── Safety hard caps (NOT user-overridable) ─────────────────────
//   Torque (Nm) = raw * 0.01 - 20.5
//   +1.80 Nm = raw 2230 = 0x8B6  → byte2 low nibble = 0x8, byte3 = 0xB6
//   -1.80 Nm = raw 1870 = 0x74E  → byte2 low nibble = 0x7, byte3 = 0x4E
static const uint16_t TORQUE_RAW_MAX = 0x8B6;
static const uint16_t TORQUE_RAW_MIN = 0x74E;
static const float    TORQUE_NM_MAX  = +1.80f;
static const float    TORQUE_NM_MIN  = -1.80f;
static const uint8_t  MAX_TORQUE_ENTRIES = 8;

// ── Modes ───────────────────────────────────────────────────────
enum NagMode : uint8_t { MODE_A = 0, MODE_B = 1, MODE_C = 2, MODE_CUSTOM = 3 };

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

// ── Live context (Mode C). Updated from canTask, read in echo. ──
struct Context {
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

// Petit log CAN global
static volatile uint32_t canAnyFrames = 0;
static unsigned long lastCanLogMs = 0;

// ── Persistence ─────────────────────────────────────────────────
static Preferences prefs;

static void cfgSetCommonDefaults(Config& c) {
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
  c.torqueCount = 4;
  c.torqueB2[0] = 0x08; c.torqueB3[0] = 0xB6;
  c.torqueB2[1] = 0x08; c.torqueB3[1] = 0x98;
  c.torqueB2[2] = 0x07; c.torqueB3[2] = 0x6C;
  c.torqueB2[3] = 0x07; c.torqueB3[3] = 0x4E;
  c.hoRatePct   = 100;
}
static void cfgDefaultsModeC(Config& c) {
  cfgSetCommonDefaults(c);
  c.mode        = MODE_C;
  c.targetId    = 0x370;
  c.torqueCount = 1;
  c.torqueB2[0] = 0x08;
  c.torqueB3[0] = 0xB6;
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
  prefs.begin("nag", true);
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
}

static void cfgSave() {
  cfgClampAll(cfg);
  prefs.begin("nag", false);
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

static bool decideInjection(const twai_message_t& src,
                            uint8_t& out_b2, uint8_t& out_b3, bool& out_setHo) {
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
  for (uint8_t i = 0; i < tCount; i++) { tB2[i] = cfg.torqueB2[i]; tB3[i] = cfg.torqueB3[i]; }
  portEXIT_CRITICAL(&cfgMux);

  if (mode == MODE_A || mode == MODE_CUSTOM) {
    static uint8_t  tIdx = 0;
    static uint16_t hoSeq = 0;
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
    static uint8_t  tIdx = 0;
    static uint32_t lastChangeMs = 0;
    if (now - lastChangeMs >= 200) { tIdx = (tIdx + 1) % tCount; lastChangeMs = now; }
    out_b2 = tB2[tIdx];
    out_b3 = tB3[tIdx];
    out_setHo = true;
    return true;
  }

  if (mode == MODE_C) {
    Context c;
    portENTER_CRITICAL(&ctxMux); c = ctx; portEXIT_CRITICAL(&ctxMux);

    const unsigned long FRESH_MS = 1000;
    if (now - c.lastApStateMs  > FRESH_MS) return false;
    if (now - c.lastSteeringMs > FRESH_MS) return false;

    if (c.apState < 3 || c.apState > 6)   return false;
    if (fabsf(c.steeringAngleDeg) > 5.0f) return false;

    float torqueNm;
    bool  setHo;
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
      portENTER_CRITICAL(&ctxMux);
      ctx.walkSeed = s;
      ctx.lastModeCTorqueNm = torqueNm;
      portEXIT_CRITICAL(&ctxMux);
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
    else {
      return false;
    }

    nmToBytes(torqueNm, out_b2, out_b3);
    out_setHo = setHo;
    return true;
  }

  return false;
}

static IRAM_ATTR void echoModified(const twai_message_t& src) {
  uint8_t b2 = 0, b3 = 0; bool setHo = false;
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
  esp_err_t err = twai_transmit(&e, pdMS_TO_TICKS(2));
  echoLatUs = micros() - t0;
  if (err == ESP_OK) {
    txOk++; echoCount++;
    lastInjectedHo = setHo ? 1 : 0;
    uint16_t raw = ((b2 & 0x0F) << 8) | b3;
    lastInjectedNm = raw * 0.01f - 20.5f;
  } else {
    txFail++;
  }
}

static void updateApState(const twai_message_t& f) {
  uint8_t apb, apsh, apmask, hob, hosh, homask;
  portENTER_CRITICAL(&cfgMux);
  apb = cfg.apStateByte; apsh = cfg.apStateShift; apmask = cfg.apStateMask;
  hob = cfg.handsOnByte; hosh = cfg.handsOnShift; homask = cfg.handsOnMask;
  portEXIT_CRITICAL(&cfgMux);
  if (apb >= 8 || hob >= 8) return;
  uint8_t ap = (f.data[apb] >> apsh) & apmask;
  uint8_t ho = (f.data[hob] >> hosh) & homask;
  unsigned long now = millis();
  portENTER_CRITICAL(&ctxMux);
  ctx.apState = ap;
  ctx.lastApStateMs = now;
  if (ho != ctx.handsOnState) {
    ctx.prevHandsOnState = ctx.handsOnState;
    ctx.handsOnState = ho;
    if (ho == 2 && ctx.state2EnterMs == 0) ctx.state2EnterMs = now;
    if (ho != 2) ctx.state2EnterMs = 0;
    if (ho == 3 && ctx.state3EnterMs == 0) ctx.state3EnterMs = now;
    if (ho != 3) ctx.state3EnterMs = 0;
  }
  portEXIT_CRITICAL(&ctxMux);
}

static void updateSteering(const twai_message_t& f) {
  uint8_t bh, bl; float scale, offs;
  portENTER_CRITICAL(&cfgMux);
  bh = cfg.steeringByteHi; bl = cfg.steeringByteLo;
  scale = cfg.steeringScale; offs = cfg.steeringOffset;
  portEXIT_CRITICAL(&cfgMux);
  if (bh >= 8 || bl >= 8) return;
  int16_t raw = (int16_t)(((uint16_t)f.data[bh] << 8) | f.data[bl]);
  float deg = raw * scale + offs;
  unsigned long now = millis();
  portENTER_CRITICAL(&ctxMux);
  ctx.steeringAngleDeg = deg;
  ctx.lastSteeringMs = now;
  portEXIT_CRITICAL(&ctxMux);
}

// ── CAN task (Core 1) ───────────────────────────────────────────
static void canTask(void* arg) {
  for (;;) {
    twai_message_t f;
    while (twai_receive(&f, pdMS_TO_TICKS(2)) == ESP_OK) {
      canAnyFrames++;

      unsigned long now = millis();
      if (now - lastCanLogMs >= 1000) {
        Serial.printf("[CAN] traffic ok total=%lu last_id=0x%03lX dlc=%u data=",
                      (unsigned long)canAnyFrames,
                      (unsigned long)f.identifier,
                      (unsigned)f.data_length_code);
        for (int i = 0; i < f.data_length_code; i++) {
          Serial.printf("%02X ", f.data[i]);
        }
        Serial.println();
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

      uint8_t ho = (f.data[4] >> 6) & 0x03;
      uint16_t tRaw = ((f.data[2] & 0x0F) << 8) | f.data[3];
      realHo     = ho;
      realTorque = tRaw * 0.01f - 20.5f;

      bool isOurs = false;
      portENTER_CRITICAL(&cfgMux);
      for (uint8_t i = 0; i < cfg.torqueCount; i++) {
        if (f.data[3] == cfg.torqueB3[i]) { isOurs = true; break; }
      }
      portEXIT_CRITICAL(&cfgMux);
      isOurs = isOurs && (ho == 1);

      if (en && !isOurs && ho <= 1) {
        echoModified(f);
      }
    }

    twai_status_info_t st; 
    twai_get_status_info(&st);
    if (st.state == TWAI_STATE_BUS_OFF) {
      twai_initiate_recovery();
      vTaskDelay(pdMS_TO_TICKS(300));
    }
    vTaskDelay(1);
  }
}

// ── HTML page (PROGMEM) ─────────────────────────────────────────
extern const char INDEX_HTML[] PROGMEM;

// ── Web server (Core 0) ─────────────────────────────────────────
static WebServer server(80);

static String cfgToJson() {
  Config c;
  portENTER_CRITICAL(&cfgMux); c = cfg; portEXIT_CRITICAL(&cfgMux);
  String s = "{";
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
  portENTER_CRITICAL(&ctxMux); c = ctx; portEXIT_CRITICAL(&ctxMux);
  String s = "{";
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
  twai_status_info_t st; twai_get_status_info(&st);
  s += ",\"canState\":";     s += String((int)st.state);
  s += "}";
  return s;
}

static void httpRoot()   { server.send_P(200, "text/html", INDEX_HTML); }
static void httpConfig() { server.send(200, "application/json", cfgToJson()); }
static void httpStats()  { server.send(200, "application/json", statsToJson()); }

static void httpSetMode() {
  int m = server.arg("m").toInt();
  Config nc;
  if      (m == 1) cfgDefaultsModeB(nc);
  else if (m == 2) cfgDefaultsModeC(nc);
  else             cfgDefaultsModeA(nc);
  portENTER_CRITICAL(&cfgMux); cfg = nc; portEXIT_CRITICAL(&cfgMux);
  cfgSave();
  server.send(200, "application/json", cfgToJson());
}

static void httpUpdate() {
  Config nc;
  portENTER_CRITICAL(&cfgMux); nc = cfg; portEXIT_CRITICAL(&cfgMux);
  if (server.hasArg("enabled"))    nc.enabled   = (server.arg("enabled") == "1");
  if (server.hasArg("targetId"))   nc.targetId  = (uint16_t)strtol(server.arg("targetId").c_str(), nullptr, 0);
  if (server.hasArg("hoRatePct"))  nc.hoRatePct = (uint8_t)server.arg("hoRatePct").toInt();
  if (server.hasArg("burstMs"))    nc.burstMs   = (uint16_t)server.arg("burstMs").toInt();
  if (server.hasArg("pauseMs"))    nc.pauseMs   = (uint16_t)server.arg("pauseMs").toInt();
  if (server.hasArg("apStateId"))  nc.apStateId = (uint16_t)strtol(server.arg("apStateId").c_str(), nullptr, 0);
  if (server.hasArg("steeringId")) nc.steeringId= (uint16_t)strtol(server.arg("steeringId").c_str(), nullptr, 0);
  if (server.hasArg("count")) {
    uint8_t n = (uint8_t)server.arg("count").toInt();
    if (n > MAX_TORQUE_ENTRIES) n = MAX_TORQUE_ENTRIES;
    if (n < 1) n = 1;
    for (uint8_t i = 0; i < n; i++) {
      String k2 = "b2_" + String(i);
      String k3 = "b3_" + String(i);
      if (server.hasArg(k2)) nc.torqueB2[i] = (uint8_t)strtol(server.arg(k2).c_str(), nullptr, 0);
      if (server.hasArg(k3)) nc.torqueB3[i] = (uint8_t)strtol(server.arg(k3).c_str(), nullptr, 0);
    }
    nc.torqueCount = n;
  }
  cfgClampAll(nc);
  portENTER_CRITICAL(&cfgMux); cfg = nc; portEXIT_CRITICAL(&cfgMux);
  cfgSave();
  server.send(200, "application/json", cfgToJson());
}

static void httpReset() {
  Config nc; cfgDefaultsModeA(nc);
  portENTER_CRITICAL(&cfgMux); cfg = nc; portEXIT_CRITICAL(&cfgMux);
  cfgSave();
  rxFrames = echoCount = txOk = txFail = 0;
  server.send(200, "application/json", cfgToJson());
}

static void webTask(void* arg) {
  WiFi.mode(WIFI_AP);
  uint8_t mac[6]; WiFi.softAPmacAddress(mac);
  char ssid[24];
  snprintf(ssid, sizeof(ssid), "Falcon-%02X%02X", mac[4], mac[5]);
  WiFi.softAP(ssid, "nagkiller");
  IPAddress ip = WiFi.softAPIP();
  Serial.printf("AP: SSID=%s PASS=nagkiller IP=%s\n", ssid, ip.toString().c_str());

  server.on("/",           HTTP_GET,  httpRoot);
  server.on("/api/config", HTTP_GET,  httpConfig);
  server.on("/api/stats",  HTTP_GET,  httpStats);
  server.on("/api/mode",   HTTP_POST, httpSetMode);
  server.on("/api/update", HTTP_POST, httpUpdate);
  server.on("/api/reset",  HTTP_POST, httpReset);
  server.begin();

  for (;;) {
    server.handleClient();
    vTaskDelay(1);
  }
}

// ── Setup ───────────────────────────────────────────────────────
void setup() {
  bootTime = millis();
  Serial.begin(115200);
  delay(500);
  Serial.printf("IDF version: %s\n", esp_get_idf_version());

  cfgLoad();
  cfgClampAll(cfg);

  twai_general_config_t g = TWAI_GENERAL_CONFIG_DEFAULT(
      (gpio_num_t)CAN_TX_PIN, (gpio_num_t)CAN_RX_PIN, TWAI_MODE_NORMAL);
  g.rx_queue_len = 64;
  g.tx_queue_len = 16;
  twai_timing_config_t t = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t f = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  esp_err_t err1 = twai_driver_install(&g, &t, &f);
  esp_err_t err2 = twai_start();
  Serial.printf("twai_driver_install=%d twai_start=%d\n", (int)err1, (int)err2);

  Serial.println("=== nag_echo v2 ===");
  Serial.printf("mode=%u id=0x%03X torqueCount=%u burst/pause=%u/%u ms enabled=%u\n",
    cfg.mode, cfg.targetId, cfg.torqueCount, cfg.burstMs, cfg.pauseMs, cfg.enabled);

  xTaskCreatePinnedToCore(canTask, "can", 4096, nullptr, 5, nullptr, 1);
  xTaskCreatePinnedToCore(webTask, "web", 8192, nullptr, 1, nullptr, 0);
}

void loop() { vTaskDelay(pdMS_TO_TICKS(1000)); }