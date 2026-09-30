#pragma once

#include <Arduino.h>
#include <Update.h>
#include <WebServer.h>
#include <WiFi.h>
#include <errno.h>
#include <stdlib.h>

#include "can_runtime.h"
#include "index_html.h"

static WebServer server(80);
static bool otaUploadStarted = false;
static bool otaUploadFailed = false;

static bool parseUIntBounded(const String &text, uint32_t minimum,
                             uint32_t maximum, uint32_t &value) {
  if (text.length() == 0u || text.length() > 10u) return false;
  errno = 0;
  char *end = nullptr;
  const unsigned long parsed = strtoul(text.c_str(), &end, 10);
  if (errno != 0 || end == text.c_str() || *end != '\0' ||
      parsed < minimum || parsed > maximum) return false;
  value = (uint32_t)parsed;
  return true;
}

static bool parseBool01(const String &text, bool &value) {
  if (text == "0") { value = false; return true; }
  if (text == "1") { value = true; return true; }
  return false;
}

static uint32_t ageOrMax(uint32_t nowMs, uint32_t timestampMs) {
  return timestampMs == 0u ? UINT32_MAX : (uint32_t)(nowMs - timestampMs);
}

static const char *modeHPhaseLabel(uint8_t phase) {
  switch (phase) {
    case H1_WAIT: return "WAIT";
    case H1_RAMP_IN: return "RAMP IN";
    case H1_INTERACT: return "INTERACT";
    case H1_RAMP_OUT: return "RAMP OUT";
    case H1_REFRACTORY: return "REFRACTORY";
    case H1_PAUSED_STOPPED: return "STOPPED";
    default: return "IDLE";
  }
}

static uint32_t modeHPhaseRemainingMs(const NagHumanV4StatePure &state,
                                      uint32_t nowMs) {
  if (!state.base.eventValid || state.base.phaseStartMs == 0u) return UINT32_MAX;
  uint32_t duration = 0u;
  switch (state.base.phase) {
    case H1_WAIT: duration = state.base.event.waitDurationMs; break;
    case H1_RAMP_IN: duration = state.base.event.rampInDurationMs; break;
    case H1_INTERACT: duration = state.base.event.interactDurationMs; break;
    case H1_RAMP_OUT: duration = state.base.event.rampOutDurationMs; break;
    case H1_REFRACTORY: duration = state.base.event.refractoryDurationMs; break;
    default: return UINT32_MAX;
  }
  const uint32_t elapsed = (uint32_t)(nowMs - state.base.phaseStartMs);
  return elapsed >= duration ? 0u : duration - elapsed;
}

static void sendJson(int status, const char *body) {
  server.sendHeader("Cache-Control", "no-store");
  server.send(status, "application/json", body);
}

static void httpRoot() {
  server.sendHeader("Content-Encoding", "gzip");
  server.sendHeader("Cache-Control", "no-cache");
  server.send_P(200, "text/html", (const char *)INDEX_HTML_GZ,
                INDEX_HTML_GZ_LEN);
}

static void httpConfig() {
  const Config c = cfgSnapshot();
  char json[1400];
  snprintf(json, sizeof(json),
      "{\"version\":\"v3.8.2-SC\",\"enabled\":%s,\"mode\":%u,"
      "\"profile\":%u,\"profileLabel\":\"%s\",\"apOnly\":%s,"
      "\"apOnlyEditable\":%s,"
      "\"hoRate\":%u,\"burstMs\":%u,\"pauseMs\":%u,"
      "\"modeH\":{\"waitMinMs\":%u,\"waitMaxMs\":%u,"
      "\"carrierMinRaw\":%u,\"carrierMaxRaw\":%u,"
      "\"visualRescueEnabled\":%s,\"visualRescueDelayMs\":%u}}",
      c.enabled ? "true" : "false", c.mode, c.profile,
      standardPartyProfileLabelPure(c.profile),
      c.apOnlyInjection ? "true" : "false",
      configApOnlyEditablePure(c) ? "true" : "false", c.hoRatePct,
      c.burstMs, c.pauseMs, c.modeH.base.waitMinMs,
      c.modeH.base.waitMaxMs, c.modeH.carrierMinRaw,
      c.modeH.carrierMaxRaw,
      c.modeH.visualRescueEnabled ? "true" : "false",
      c.modeH.visualRescueDelayMs);
  sendJson(200, json);
}

static void httpStatus() {
  const CanRuntimeSnapshot s = canRuntimeSnapshot();
  const uint32_t nowMs = millis();
  const uint16_t dasIdRaw = dasSourceLockedIdPure(s.runtime.dasSource);
  char dasId[12];
  if (dasIdRaw != 0u) snprintf(dasId, sizeof(dasId), "0x%03X", dasIdRaw);
  else snprintf(dasId, sizeof(dasId), "SCANNING");
  const uint32_t epasAge = ageOrMax(nowMs, s.runtime.live.lastEpasMs);
  const uint32_t dasAge = ageOrMax(nowMs, s.runtime.live.lastDasMs);
  const uint32_t speedAge = ageOrMax(nowMs, s.runtime.live.lastSpeedMs);
  const uint32_t nextMs = modeHPhaseRemainingMs(s.runtime.modeH, nowMs);
  const bool effectiveOn = s.config.enabled && s.runtime.restore.ready;
  NagSafetyInputPure statusInput = {};
  statusInput.profileSelected = s.config.profile != PROFILE_UNSET;
  statusInput.requestedOn = s.config.enabled;
  statusInput.effectiveOn = effectiveOn;
  statusInput.twaiReady = s.ready && !s.recovering;
  statusInput.otaActive = s.ota;
  statusInput.epasFresh = s.runtime.live.epasValid &&
      epasAge <= EPAS_FRESH_MS;
  statusInput.selfEcho = false;
  statusInput.stockHandsOnSupported = s.runtime.live.epasHandsOn <= 1u;
  statusInput.torqueWithinLimit = true;
  statusInput.apOnlyInjection = s.config.apOnlyInjection;
  statusInput.dasRequired = configDasRequiredPure(s.config);
  statusInput.dasValid = s.runtime.live.dasValid;
  statusInput.dasFresh = dasAge <= DAS_FRESH_MS;
  statusInput.apActive = canApActive(s.runtime.live.apState);
  statusInput.modeH = s.config.mode == MODE_H;
  statusInput.speedValid = s.runtime.live.speedValid;
  statusInput.speedFresh = speedAge <= SPEED_FRESH_MS;
  const NagSafetyDecisionPure statusGate = nagEvaluateSafetyPure(statusInput);
  const bool txActive = statusGate.allowed &&
      s.runtime.diagnostics.blockReason == NAG_BLOCK_NONE &&
      s.runtime.diagnostics.lastTxMs != 0u &&
      (uint32_t)(nowMs - s.runtime.diagnostics.lastTxMs) <= 1000u;
  const uint8_t statusReason = statusGate.allowed
      ? s.runtime.diagnostics.blockReason : statusGate.reason;
  const char *block = s.recovering ? "CAN RECOVERY" :
      nagBlockReasonLabelPure(statusReason);
  char json[2400];
  snprintf(json, sizeof(json),
      "{\"requestedOn\":%s,\"effectiveOn\":%s,\"txActive\":%s,"
      "\"restoreRemainingMs\":%lu,\"restorePhase\":%u,"
      "\"blockReason\":\"%s\",\"profile\":%u,"
      "\"profileLabel\":\"%s\",\"dasId\":\"%s\","
      "\"dasValid\":%s,\"dasAgeMs\":%lu,\"apState\":%u,"
      "\"visualState\":%u,\"speedValid\":%s,"
      "\"speedKphX100\":%ld,\"speedAgeMs\":%lu,"
      "\"epasValid\":%s,\"epasAgeMs\":%lu,\"epasTorqueRaw\":%u,"
      "\"modeHPhase\":\"%s\",\"modeHNextMs\":%lu,"
      "\"visualRescuePending\":%s,\"modeHEvents\":%lu,"
      "\"visualRescueCount\":%lu,\"rxFrames\":%lu,\"txOk\":%lu,"
      "\"txFail\":%lu,\"selfEchoFrames\":%lu,\"recoveryCount\":%lu,"
      "\"recoveryFailCount\":%lu,\"canReady\":%s,"
      "\"canRecovering\":%s,\"otaActive\":%s,\"canState\":%d,"
      "\"epoch\":%lu}",
      s.config.enabled ? "true" : "false",
      effectiveOn ? "true" : "false",
      txActive ? "true" : "false",
      (unsigned long)s.runtime.restoreRemainingMs, s.runtime.restorePhase,
      block, s.config.profile, standardPartyProfileLabelPure(s.config.profile),
      dasId, s.runtime.live.dasValid ? "true" : "false",
      (unsigned long)dasAge, s.runtime.live.apState,
      s.runtime.live.visualState,
      s.runtime.live.speedValid ? "true" : "false",
      (long)s.runtime.live.speedKphX100, (unsigned long)speedAge,
      s.runtime.live.epasValid ? "true" : "false",
      (unsigned long)epasAge, s.runtime.live.epasTorqueRaw,
      modeHPhaseLabel(s.runtime.modeH.base.phase), (unsigned long)nextMs,
      s.runtime.modeH.visualRescuePending ? "true" : "false",
      (unsigned long)s.runtime.modeH.base.eventCount,
      (unsigned long)s.runtime.modeH.visualRescueCount,
      (unsigned long)s.runtime.diagnostics.rxFrames,
      (unsigned long)s.runtime.diagnostics.txOk,
      (unsigned long)s.runtime.diagnostics.txFail,
      (unsigned long)s.runtime.diagnostics.selfEchoFrames,
      (unsigned long)s.runtime.diagnostics.recoveryCount,
      (unsigned long)s.runtime.diagnostics.recoveryFailCount,
      s.ready ? "true" : "false", s.recovering ? "true" : "false",
      s.ota ? "true" : "false", s.canState, (unsigned long)s.epoch);
  sendJson(200, json);
}

static void httpSetNag() {
  bool enabled = false;
  if (!server.hasArg("enabled") || !parseBool01(server.arg("enabled"), enabled)) {
    server.send(400, "application/json", "{\"error\":\"enabled must be 0 or 1\"}");
    return;
  }
  Config candidate = cfgSnapshot();
  if (enabled && candidate.profile == PROFILE_UNSET) {
    server.send(400, "application/json", "{\"error\":\"Select a vehicle profile first\"}");
    return;
  }
  candidate.enabled = enabled;
  cfgCommit(candidate, RUNTIME_RESET_MASTER);
  sendJson(200, "{\"ok\":true}");
}

static void httpSetMode() {
  uint32_t mode = UINT32_MAX;
  if (!server.hasArg("mode") || !parseUIntBounded(server.arg("mode"), 0u, 255u, mode) ||
      mode < MODE_A || mode > MODE_H) {
    server.send(400, "application/json", "{\"error\":\"mode must be 0 through 3\"}");
    return;
  }
  Config candidate = cfgSnapshot();
  configApplyModeDefaultsPure(candidate, (uint8_t)mode);
  cfgCommit(candidate, RUNTIME_RESET_MODE);
  sendJson(200, "{\"ok\":true}");
}

static void httpSetProfile() {
  uint32_t profile = UINT32_MAX;
  if (!server.hasArg("profile") ||
      !parseUIntBounded(server.arg("profile"), 0u, 255u, profile) ||
      !standardPartyProfileValidPure((uint8_t)profile)) {
    server.send(400, "application/json", "{\"error\":\"profile must be 0 through 3\"}");
    return;
  }
  Config candidate = cfgSnapshot();
  configApplyProfilePure(candidate, (uint8_t)profile);
  cfgCommit(candidate, RUNTIME_RESET_PROFILE);
  sendJson(200, "{\"ok\":true}");
}

static void httpSetSettings() {
  Config candidate = cfgSnapshot();
  uint32_t value = 0u;
  bool flag = false;
  if (server.hasArg("apOnly")) {
    if (!parseBool01(server.arg("apOnly"), flag)) {
      server.send(400, "application/json", "{\"error\":\"apOnly must be 0 or 1\"}"); return;
    }
    candidate.apOnlyInjection = flag;
  }
  if (server.hasArg("hoRate")) {
    if (!parseUIntBounded(server.arg("hoRate"), 0u, 100u, value)) {
      server.send(400, "application/json", "{\"error\":\"hoRate must be 0 through 100\"}"); return;
    }
    candidate.hoRatePct = (uint8_t)value;
  }
  if (server.hasArg("burstMs")) {
    if (!parseUIntBounded(server.arg("burstMs"), 50u, 10000u, value)) {
      server.send(400, "application/json", "{\"error\":\"burstMs must be 50 through 10000\"}"); return;
    }
    candidate.burstMs = (uint16_t)value;
  }
  if (server.hasArg("pauseMs")) {
    if (!parseUIntBounded(server.arg("pauseMs"), 0u, 10000u, value)) {
      server.send(400, "application/json", "{\"error\":\"pauseMs must be 0 through 10000\"}"); return;
    }
    candidate.pauseMs = (uint16_t)value;
  }
  if (server.hasArg("waitMinMs")) {
    if (!parseUIntBounded(server.arg("waitMinMs"), 0u, 65535u, value)) {
      server.send(400, "application/json", "{\"error\":\"Invalid waitMinMs\"}"); return;
    }
    candidate.modeH.base.waitMinMs = (uint16_t)value;
  }
  if (server.hasArg("waitMaxMs")) {
    if (!parseUIntBounded(server.arg("waitMaxMs"), 0u, 65535u, value)) {
      server.send(400, "application/json", "{\"error\":\"Invalid waitMaxMs\"}"); return;
    }
    candidate.modeH.base.waitMaxMs = (uint16_t)value;
  }
  if (server.hasArg("carrierMinRaw")) {
    if (!parseUIntBounded(server.arg("carrierMinRaw"), 0u, 65535u, value)) {
      server.send(400, "application/json", "{\"error\":\"Invalid carrierMinRaw\"}"); return;
    }
    candidate.modeH.carrierMinRaw = (uint16_t)value;
  }
  if (server.hasArg("carrierMaxRaw")) {
    if (!parseUIntBounded(server.arg("carrierMaxRaw"), 0u, 65535u, value)) {
      server.send(400, "application/json", "{\"error\":\"Invalid carrierMaxRaw\"}"); return;
    }
    candidate.modeH.carrierMaxRaw = (uint16_t)value;
  }
  if (server.hasArg("visualRescueEnabled")) {
    if (!parseBool01(server.arg("visualRescueEnabled"), flag)) {
      server.send(400, "application/json", "{\"error\":\"visualRescueEnabled must be 0 or 1\"}"); return;
    }
    candidate.modeH.visualRescueEnabled = flag;
  }
  if (server.hasArg("visualRescueDelayMs")) {
    if (!parseUIntBounded(server.arg("visualRescueDelayMs"), 0u, 65535u, value)) {
      server.send(400, "application/json", "{\"error\":\"Invalid visualRescueDelayMs\"}"); return;
    }
    candidate.modeH.visualRescueDelayMs = (uint16_t)value;
  }
  if (!configValidatePure(candidate) || !nagHumanV4ConfigValidPure(candidate.modeH)) {
    server.send(400, "application/json", "{\"error\":\"Settings are outside safe limits\"}");
    return;
  }
  cfgCommit(candidate, RUNTIME_RESET_AP_GATE);
  sendJson(200, "{\"ok\":true}");
}

static void httpRestart() {
  sendJson(200, "{\"ok\":true,\"restarting\":true}");
  delay(200);
  ESP.restart();
}

static void httpFactoryReset() {
  const bool cleared = cfgClearPersistent();
  if (!cleared) {
    server.send(500, "application/json", "{\"error\":\"Factory reset failed\"}");
    return;
  }
  cfgCommit(configDefaultsPure(), RUNTIME_RESET_FACTORY);
  sendJson(200, "{\"ok\":true,\"restarting\":true}");
  delay(200);
  ESP.restart();
}

static void httpUpdateFinish() {
  const bool ok = otaUploadStarted && !otaUploadFailed && !Update.hasError() &&
                  Update.end(true);
  if (!ok) {
    otaInProgress = false;
    server.send(500, "application/json", "{\"error\":\"Firmware update failed\"}");
    return;
  }
  sendJson(200, "{\"ok\":true,\"restarting\":true}");
  delay(250);
  ESP.restart();
}

static void httpUpdateUpload() {
  HTTPUpload &upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    otaInProgress = true;
    canRuntimeInvalidate(RUNTIME_RESET_OTA);
    otaUploadStarted = Update.begin(UPDATE_SIZE_UNKNOWN);
    otaUploadFailed = !otaUploadStarted;
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (!otaUploadStarted || Update.write(upload.buf, upload.currentSize) !=
        upload.currentSize) otaUploadFailed = true;
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    otaUploadFailed = true;
    Update.abort();
    otaInProgress = false;
  }
}

static void webBegin() {
  uint64_t chip = ESP.getEfuseMac();
  char ssid[24];
  snprintf(ssid, sizeof(ssid), "NAG-KILLER-%04X", (unsigned)(chip & 0xFFFFu));
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, "12345678");
  server.on("/", HTTP_GET, httpRoot);
  server.on("/api/config", HTTP_GET, httpConfig);
  server.on("/api/status", HTTP_GET, httpStatus);
  server.on("/api/nag", HTTP_POST, httpSetNag);
  server.on("/api/mode", HTTP_POST, httpSetMode);
  server.on("/api/profile", HTTP_POST, httpSetProfile);
  server.on("/api/settings", HTTP_POST, httpSetSettings);
  server.on("/api/restart", HTTP_POST, httpRestart);
  server.on("/api/factory-reset", HTTP_POST, httpFactoryReset);
  server.on("/update", HTTP_POST, httpUpdateFinish, httpUpdateUpload);
  server.onNotFound([]() { server.send(404, "application/json", "{\"error\":\"Not found\"}"); });
  server.begin();
}

static void webTask(void *arg) {
  (void)arg;
  webBegin();
  for (;;) {
    server.handleClient();
    vTaskDelay(pdMS_TO_TICKS(2));
  }
}
