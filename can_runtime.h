#pragma once

#include <Arduino.h>
#include <esp_system.h>
#include "driver/twai.h"

#include "app_state.h"
#include "epas_370_pure.h"

static constexpr int CAN_TX_PIN = 5;
static constexpr int CAN_RX_PIN = 6;
static constexpr uint16_t EPAS_370_ID = 0x370;
static constexpr uint16_t PARTY_SPEED_ID = 0x257;
static constexpr uint32_t DAS_FRESH_MS = 1000u;
static constexpr uint32_t SPEED_FRESH_MS = 1000u;
static constexpr uint32_t EPAS_FRESH_MS = 1000u;
static constexpr uint32_t EXACT_ECHO_FRESH_MS = 20u;
static constexpr uint32_t TWAI_STATUS_POLL_MS = 250u;
static constexpr uint32_t TWAI_RECOVERY_TIMEOUT_MS = 1800u;
static constexpr uint16_t MODE_C_RAW_MIN = 0x898u;
static constexpr uint16_t MODE_C_RAW_MAX = 0x8B6u;
static constexpr uint16_t MODE_C_MAX_STEP = 15u;
static constexpr uint16_t CAN_RX_BATCH_MAX = 32u;

struct CanRuntimeSnapshot {
  Config config;
  AppRuntimeState runtime;
  bool ready;
  bool recovering;
  bool ota;
  uint32_t epoch;
  int canState;
};

struct NagInjectionDecision {
  bool tx;
  uint16_t torqueRaw;
  bool hoOverrideValid;
  uint8_t hoLevel;
};

static uint32_t canLastStatusPollMs = 0u;
static uint32_t canRecoveryStartMs = 0u;

static inline bool canApActive(uint8_t state) {
  return state == 3u || state == 4u || state == 5u || state == 6u;
}

static void canRuntimeRecordBlock(uint8_t reason, uint32_t nowMs) {
  portENTER_CRITICAL(&runtimeMux);
  appRuntime.diagnostics.blockReason = reason;
  appRuntime.diagnostics.lastBlockMs = nowMs;
  appRuntime.diagnostics.rejectedFrames++;
  portEXIT_CRITICAL(&runtimeMux);
}

static void canRuntimeInvalidate(NagRuntimeResetReason reason) {
  const bool locked = canTxMutex &&
      xSemaphoreTake(canTxMutex, portMAX_DELAY) == pdTRUE;
  runtimeInvalidate(reason);
  if (locked) xSemaphoreGive(canTxMutex);
}

static CanRuntimeSnapshot canRuntimeSnapshot() {
  CanRuntimeSnapshot snapshot = {};
  snapshot.config = cfgSnapshot();
  portENTER_CRITICAL(&runtimeMux);
  snapshot.runtime = appRuntime;
  portEXIT_CRITICAL(&runtimeMux);
  snapshot.ready = twaiReady;
  snapshot.recovering = twaiRecovering;
  snapshot.ota = otaInProgress;
  snapshot.epoch = runtimeEpochSnapshot();
  twai_status_info_t status = {};
  snapshot.canState = twai_get_status_info(&status) == ESP_OK
      ? (int)status.state : -1;
  return snapshot;
}

static bool canRuntimeInstallAndStart() {
  twai_general_config_t general = TWAI_GENERAL_CONFIG_DEFAULT(
      (gpio_num_t)CAN_TX_PIN, (gpio_num_t)CAN_RX_PIN, TWAI_MODE_NORMAL);
  general.rx_queue_len = 256;
  general.tx_queue_len = 16;
  const twai_timing_config_t timing = TWAI_TIMING_CONFIG_500KBITS();
  const twai_filter_config_t filter = TWAI_FILTER_CONFIG_ACCEPT_ALL();
  const esp_err_t installed = twai_driver_install(&general, &timing, &filter);
  if (installed != ESP_OK) return false;
  if (twai_start() != ESP_OK) {
    (void)twai_driver_uninstall();
    return false;
  }
  return true;
}

static bool canRuntimeBegin() {
  if (!canTxMutexEnsure()) return false;
  twaiReady = false;
  twaiRecovering = false;
  if (!canRuntimeInstallAndStart()) return false;
  canInitTimeMs = millis();
  twaiReady = true;
  canRuntimeInvalidate(RUNTIME_RESET_CAN_RECOVERY);
  return true;
}

static void canRuntimeUpdateDas(const uint8_t *data, uint8_t dlc,
                                uint32_t nowMs) {
  const DasStatusPure decoded = dasStatusDecodePure(data, dlc);
  if (!decoded.valid) return;
  portENTER_CRITICAL(&runtimeMux);
  appRuntime.live.dasValid = true;
  appRuntime.live.apState = decoded.apState;
  appRuntime.live.visualState = decoded.visualState;
  appRuntime.live.lastDasMs = nowMs;
  nagVisualWarningUpdatePure(appRuntime.live.visual, decoded.visualState, nowMs);
  portEXIT_CRITICAL(&runtimeMux);
}

static void canRuntimeUpdateSpeed(const uint8_t *data, uint8_t dlc,
                                  uint32_t nowMs) {
  const PartySpeedPure decoded = partySpeedDecodePure(data, dlc);
  portENTER_CRITICAL(&runtimeMux);
  appRuntime.live.speedValid = decoded.valid;
  if (decoded.valid) {
    appRuntime.live.speedRaw = decoded.raw;
    appRuntime.live.speedKphX100 = decoded.kphX100;
  }
  appRuntime.live.lastSpeedMs = nowMs;
  portEXIT_CRITICAL(&runtimeMux);
}

static bool canRuntimeLooksLikeLegacyEcho(const Config &config,
                                          const Epas370SourcePure &source) {
  if (source.handsOn != 1u) return false;
  if (config.mode == MODE_C) {
    portENTER_CRITICAL(&runtimeMux);
    const uint16_t raw = appRuntime.modeCRaw;
    portEXIT_CRITICAL(&runtimeMux);
    return raw != 0u && source.torqueRaw == raw;
  }
  for (uint8_t i = 0u; i < config.torqueCount; ++i) {
    if (source.torqueRaw == configTorqueRawPure(config, i)) return true;
  }
  return false;
}

static uint16_t canRuntimeModeCNext(uint16_t previous, uint16_t &seed) {
  if (seed == 0u) seed = 0xACE1u;
  seed = (uint16_t)(seed * 25173u + 13849u);
  int32_t next = (int32_t)previous + (int32_t)(seed % 31u) - 15;
  if (next < MODE_C_RAW_MIN) next = MODE_C_RAW_MIN;
  if (next > MODE_C_RAW_MAX) next = MODE_C_RAW_MAX;
  return (uint16_t)next;
}

static NagInjectionDecision canRuntimeLegacyDecision(const Config &config,
                                                      uint32_t nowMs) {
  NagInjectionDecision out = {};
  portENTER_CRITICAL(&runtimeMux);
  if (config.mode == MODE_A) {
    const uint8_t index = (uint8_t)(appRuntime.modeTorqueIndex % config.torqueCount);
    appRuntime.modeTorqueIndex++;
    out.torqueRaw = configTorqueRawPure(config, index);
    const bool setHo = ((uint32_t)appRuntime.hoSequence * 100u) / 65536u <
                       config.hoRatePct;
    appRuntime.hoSequence =
        (uint16_t)(appRuntime.hoSequence * 1103u + 12345u);
    out.hoOverrideValid = setHo;
    out.hoLevel = setHo ? 1u : 0u;
    out.tx = true;
  } else if (config.mode == MODE_B) {
    if (appRuntime.modeBPhaseStartMs == 0u)
      appRuntime.modeBPhaseStartMs = nowMs;
    const uint32_t cycle = (uint32_t)config.burstMs + config.pauseMs;
    const uint32_t phase = cycle == 0u ? 0u :
        (uint32_t)(nowMs - appRuntime.modeBPhaseStartMs) % cycle;
    if (phase < config.burstMs) {
      if ((uint32_t)(nowMs - appRuntime.modeStepMs) >= 200u) {
        appRuntime.modeTorqueIndex =
            (uint8_t)((appRuntime.modeTorqueIndex + 1u) % config.torqueCount);
        appRuntime.modeStepMs = nowMs;
      }
      out.torqueRaw = configTorqueRawPure(config, appRuntime.modeTorqueIndex);
      out.hoOverrideValid = true;
      out.hoLevel = 1u;
      out.tx = true;
    }
  } else if (config.mode == MODE_C) {
    if (appRuntime.modeBPhaseStartMs == 0u)
      appRuntime.modeBPhaseStartMs = nowMs;
    const uint32_t cycle = (uint32_t)config.burstMs + config.pauseMs;
    const uint32_t phase = cycle == 0u ? 0u :
        (uint32_t)(nowMs - appRuntime.modeBPhaseStartMs) % cycle;
    if (config.pauseMs == 0u || phase < config.burstMs) {
      if (appRuntime.modeCSeed == 0u)
        appRuntime.modeCSeed = (uint16_t)(esp_random() & 0xFFFFu);
      if (appRuntime.modeCRaw < MODE_C_RAW_MIN ||
          appRuntime.modeCRaw > MODE_C_RAW_MAX)
        appRuntime.modeCRaw = 0x8A7u;
      if ((uint32_t)(nowMs - appRuntime.modeStepMs) >= 200u) {
        appRuntime.modeCRaw = canRuntimeModeCNext(
            appRuntime.modeCRaw, appRuntime.modeCSeed);
        appRuntime.modeStepMs = nowMs;
      }
      out.torqueRaw = appRuntime.modeCRaw;
      out.hoOverrideValid = true;
      out.hoLevel = 1u;
      out.tx = true;
    }
  }
  portEXIT_CRITICAL(&runtimeMux);
  return out;
}

static NagInjectionDecision canRuntimeModeHDecision(const Config &config,
                                                     uint32_t nowMs,
                                                     uint16_t sourceRaw) {
  NagInjectionDecision out = {};
  portENTER_CRITICAL(&runtimeMux);
  if (!appRuntime.modeHInitialized) {
    nagHumanV4InitPure(appRuntime.modeH, esp_random() ^ nowMs ^ 0x53434E34u);
    appRuntime.modeHInitialized = true;
  }
  const bool speedFresh = appRuntime.live.lastSpeedMs != 0u &&
      (uint32_t)(nowMs - appRuntime.live.lastSpeedMs) <= SPEED_FRESH_MS;
  const NagHumanV1StepResultPure result = modeHRev4ScStepPure(
      appRuntime.modeH, config.modeH, nowMs, sourceRaw, true,
      appRuntime.live.speedValid, speedFresh, appRuntime.live.speedRaw,
      appRuntime.live.visual);
  appRuntime.lastModeHDecision = result;
  out.tx = result.tx;
  out.torqueRaw = result.raw;
  out.hoOverrideValid = result.hoOverrideValid;
  out.hoLevel = result.hoLevel > 2u ? 2u : result.hoLevel;
  portEXIT_CRITICAL(&runtimeMux);
  return out;
}

static bool canRuntimeRestorePrerequisites(const Config &config,
                                           const AppLiveContext &live,
                                           uint32_t nowMs) {
  const bool dasFresh = live.dasValid && live.lastDasMs != 0u &&
      (uint32_t)(nowMs - live.lastDasMs) <= DAS_FRESH_MS;
  const bool speedFresh = live.speedValid && live.lastSpeedMs != 0u &&
      (uint32_t)(nowMs - live.lastSpeedMs) <= SPEED_FRESH_MS;
  NagRestorePrerequisitesInputPure input = {};
  input.profileSelected = config.profile != PROFILE_UNSET;
  input.twaiReady = twaiReady && !twaiRecovering;
  input.otaActive = otaInProgress;
  input.epasValid = live.epasValid && live.lastEpasMs != 0u &&
      (uint32_t)(nowMs - live.lastEpasMs) <= EPAS_FRESH_MS;
  input.stockHandsOnSupported = live.epasHandsOn <= 1u;
  input.dasRequired = configDasRequiredPure(config);
  input.dasFresh = dasFresh;
  input.speedRequired = config.mode == MODE_H;
  input.speedFresh = speedFresh;
  return nagRestorePrerequisitesReadyPure(input);
}

static NagSafetyInputPure canRuntimeSafetyInput(const Config &config,
                                                const AppLiveContext &live,
                                                uint32_t nowMs,
                                                bool effectiveOn) {
  NagSafetyInputPure input = {};
  input.profileSelected = config.profile != PROFILE_UNSET;
  input.requestedOn = config.enabled;
  input.effectiveOn = effectiveOn;
  input.twaiReady = twaiReady && !twaiRecovering;
  input.otaActive = otaInProgress;
  input.epasFresh = live.epasValid && live.lastEpasMs != 0u &&
      (uint32_t)(nowMs - live.lastEpasMs) <= EPAS_FRESH_MS;
  input.selfEcho = false;
  input.stockHandsOnSupported = live.epasHandsOn <= 1u;
  input.torqueWithinLimit = true;
  input.apOnlyInjection = config.apOnlyInjection;
  input.dasRequired = configDasRequiredPure(config);
  input.dasValid = live.dasValid;
  input.dasFresh = live.lastDasMs != 0u &&
      (uint32_t)(nowMs - live.lastDasMs) <= DAS_FRESH_MS;
  input.apActive = canApActive(live.apState);
  input.modeH = config.mode == MODE_H;
  input.speedValid = live.speedValid;
  input.speedFresh = live.lastSpeedMs != 0u &&
      (uint32_t)(nowMs - live.lastSpeedMs) <= SPEED_FRESH_MS;
  return input;
}

static bool canRuntimeUpdateRestore(uint32_t nowMs) {
  // Config commits and runtime invalidation hold this same mutex. Snapshot and
  // update together so a tick cannot restore an earlier configuration epoch.
  if (!canTxMutex ||
      xSemaphoreTake(canTxMutex, pdMS_TO_TICKS(10)) != pdTRUE) return false;
  const Config config = cfgSnapshot();
  portENTER_CRITICAL(&runtimeMux);
  const bool prerequisites = canRuntimeRestorePrerequisites(
      config, appRuntime.live, nowMs);
  const NagRestoreResultPure result = nagRestoreUpdatePure(
      appRuntime.restore, nowMs, config.enabled, prerequisites,
      NAG_RESTORE_DELAY_MS);
  appRuntime.effectiveEnabled = result.effectiveEnabled;
  appRuntime.restoreRemainingMs = result.remainingMs;
  appRuntime.restorePhase = result.phase;
  const NagSafetyInputPure input = canRuntimeSafetyInput(
      config, appRuntime.live, nowMs, result.effectiveEnabled);
  appRuntime.diagnostics.blockReason = nagRestoreBlockReasonPure(input);
  if (appRuntime.diagnostics.blockReason != NAG_BLOCK_NONE)
    appRuntime.diagnostics.lastBlockMs = nowMs;
  portEXIT_CRITICAL(&runtimeMux);
  xSemaphoreGive(canTxMutex);
  return true;
}

static void canRuntimeProcessEpas(const twai_message_t &frame,
                                  const Config &config, uint32_t nowMs,
                                  uint32_t decisionEpoch) {
  // Observe an expired previous source before replacing its timestamp, even if
  // this is the first received frame after a long interruption.
  if (!canRuntimeUpdateRestore(nowMs)) return;
  Epas370SourcePure source = {};
  if (!epas370DecodeSourcePure(frame.data, frame.data_length_code, source)) {
    canRuntimeRecordBlock(NAG_BLOCK_EPAS_STALE, nowMs);
    return;
  }

  bool exactValid;
  uint32_t exactMs;
  uint8_t exactData[8];
  portENTER_CRITICAL(&runtimeMux);
  exactValid = appRuntime.exactEchoValid;
  exactMs = appRuntime.exactEchoMs;
  memcpy(exactData, appRuntime.exactEchoData, sizeof(exactData));
  portEXIT_CRITICAL(&runtimeMux);
  const uint32_t exactAge = exactMs == 0u ? UINT32_MAX :
      (uint32_t)(nowMs - exactMs);
  const bool exactEcho = epas370ExactRecentEchoPure(
      config.mode == MODE_H, exactValid, exactAge,
      frame.data, exactData, frame.data_length_code);
  const bool legacyEcho = config.mode != MODE_H &&
      canRuntimeLooksLikeLegacyEcho(config, source);
  if (exactEcho || legacyEcho) {
    portENTER_CRITICAL(&runtimeMux);
    appRuntime.diagnostics.selfEchoFrames++;
    portEXIT_CRITICAL(&runtimeMux);
    return;
  }

  portENTER_CRITICAL(&runtimeMux);
  if (runtimeEpoch != decisionEpoch) {
    portEXIT_CRITICAL(&runtimeMux);
    return;
  }
  appRuntime.live.epasValid = true;
  memcpy(appRuntime.live.epasData, frame.data, 8u);
  appRuntime.live.epasTorqueRaw = source.torqueRaw;
  appRuntime.live.epasHandsOn = source.handsOn;
  appRuntime.live.lastEpasMs = nowMs;
  appRuntime.diagnostics.epasRxFrames++;
  portEXIT_CRITICAL(&runtimeMux);

  canRuntimeUpdateRestore(nowMs);
  portENTER_CRITICAL(&runtimeMux);
  const AppLiveContext live = appRuntime.live;
  const bool effectiveOn = appRuntime.effectiveEnabled;
  portEXIT_CRITICAL(&runtimeMux);
  const NagSafetyInputPure gateInput = canRuntimeSafetyInput(
      config, live, nowMs, effectiveOn);
  const uint8_t reason = nagRestoreBlockReasonPure(gateInput);
  const NagSafetyDecisionPure gate = {reason == NAG_BLOCK_NONE, reason};
  if (!gate.allowed) {
    if (config.mode == MODE_H) {
      portENTER_CRITICAL(&runtimeMux);
      if (appRuntime.modeHInitialized)
        nagHumanV4ResetRuntimePure(appRuntime.modeH, H1_IDLE);
      portEXIT_CRITICAL(&runtimeMux);
    }
    canRuntimeRecordBlock(gate.reason, nowMs);
    return;
  }

  const NagInjectionDecision injection = config.mode == MODE_H
      ? canRuntimeModeHDecision(config, nowMs, source.torqueRaw)
      : canRuntimeLegacyDecision(config, nowMs);
  if (!injection.tx) return;
  if (injection.torqueRaw < SC_TORQUE_RAW_MIN ||
      injection.torqueRaw > SC_TORQUE_RAW_MAX) {
    canRuntimeRecordBlock(NAG_BLOCK_TORQUE_LIMIT, nowMs);
    return;
  }

  twai_message_t tx = {};
  tx.identifier = EPAS_370_ID;
  tx.data_length_code = 8u;
  tx.flags = 0u;
  if (!epas370ComposePure(frame.data, frame.data_length_code,
                          injection.torqueRaw, injection.hoOverrideValid,
                          injection.hoLevel, tx.data)) return;

  if (!canTxMutex ||
      xSemaphoreTake(canTxMutex, pdMS_TO_TICKS(10)) != pdTRUE) {
    canRuntimeRecordBlock(NAG_BLOCK_TWAI_NOT_READY, nowMs);
    return;
  }
  esp_err_t result = ESP_ERR_INVALID_STATE;
  if (runtimeEpochStillCurrent(decisionEpoch) && twaiReady && !otaInProgress)
    result = twai_transmit(&tx, pdMS_TO_TICKS(2));
  xSemaphoreGive(canTxMutex);

  portENTER_CRITICAL(&runtimeMux);
  if (result == ESP_OK) {
    appRuntime.diagnostics.txOk++;
    appRuntime.diagnostics.lastTxMs = nowMs;
    appRuntime.diagnostics.blockReason = NAG_BLOCK_NONE;
    if (config.mode == MODE_H) {
      appRuntime.exactEchoValid = true;
      memcpy(appRuntime.exactEchoData, tx.data, 8u);
      appRuntime.exactEchoMs = nowMs;
    }
  } else {
    appRuntime.diagnostics.txFail++;
  }
  portEXIT_CRITICAL(&runtimeMux);
}

static void canRuntimeProcessFrame(const twai_message_t &frame,
                                   uint32_t nowMs) {
  const uint32_t decisionEpoch = runtimeEpochSnapshot();
  const Config config = cfgSnapshot();
  portENTER_CRITICAL(&runtimeMux);
  appRuntime.diagnostics.rxFrames++;
  appRuntime.diagnostics.lastCanFrameMs = nowMs;
  portEXIT_CRITICAL(&runtimeMux);

  if (frame.identifier == DAS_SOURCE_399_ID ||
      frame.identifier == DAS_SOURCE_39B_ID) {
    DasSourceDecisionPure decision;
    portENTER_CRITICAL(&runtimeMux);
    decision = dasSourceObservePure(
        appRuntime.dasSource, frame.identifier, frame.data,
        frame.data_length_code, nowMs, frame.extd != 0u, frame.rtr != 0u);
    portEXIT_CRITICAL(&runtimeMux);
    if (decision == DAS_SOURCE_LOCKED_NOW ||
        decision == DAS_SOURCE_ACCEPTED)
      canRuntimeUpdateDas(frame.data, frame.data_length_code, nowMs);
    return;
  }
  if (frame.identifier == PARTY_SPEED_ID) {
    canRuntimeUpdateSpeed(frame.data, frame.data_length_code, nowMs);
    return;
  }
  if (frame.identifier == EPAS_370_ID)
    canRuntimeProcessEpas(frame, config, nowMs, decisionEpoch);
}

static void canRuntimeRecoveryBoundary() {
  const bool locked = canTxMutex &&
      xSemaphoreTake(canTxMutex, portMAX_DELAY) == pdTRUE;
  twaiReady = false;
  runtimeInvalidate(RUNTIME_RESET_CAN_RECOVERY);
  portENTER_CRITICAL(&runtimeMux);
  appRuntime.diagnostics.recoveryCount++;
  portEXIT_CRITICAL(&runtimeMux);
  if (locked) xSemaphoreGive(canTxMutex);
}

static void canRuntimePollRecovery(uint32_t nowMs) {
  if ((uint32_t)(nowMs - canLastStatusPollMs) < TWAI_STATUS_POLL_MS) return;
  canLastStatusPollMs = nowMs;
  twai_status_info_t status = {};
  if (twai_get_status_info(&status) != ESP_OK) return;
  if (status.state == TWAI_STATE_RUNNING) {
    twaiReady = true;
    twaiRecovering = false;
    return;
  }
  if (status.state == TWAI_STATE_BUS_OFF && !twaiRecovering) {
    canRuntimeRecoveryBoundary();
    twaiRecovering = true;
    canRecoveryStartMs = nowMs;
    if (twai_initiate_recovery() != ESP_OK) {
      portENTER_CRITICAL(&runtimeMux);
      appRuntime.diagnostics.recoveryFailCount++;
      portEXIT_CRITICAL(&runtimeMux);
    }
    return;
  }
  if (status.state == TWAI_STATE_STOPPED && twaiRecovering) {
    if (twai_start() == ESP_OK) {
      twaiRecovering = false;
      twaiReady = true;
      canInitTimeMs = nowMs;
      canRuntimeInvalidate(RUNTIME_RESET_CAN_RECOVERY);
    }
    return;
  }
  if (status.state == TWAI_STATE_RECOVERING) twaiReady = false;
  if (twaiRecovering &&
      (uint32_t)(nowMs - canRecoveryStartMs) >= TWAI_RECOVERY_TIMEOUT_MS) {
    (void)twai_stop();
    const esp_err_t uninstalled = twai_driver_uninstall();
    const bool restarted = uninstalled == ESP_OK && canRuntimeInstallAndStart();
    twaiRecovering = false;
    twaiReady = restarted;
    if (restarted) {
      canInitTimeMs = nowMs;
      canRuntimeInvalidate(RUNTIME_RESET_CAN_RECOVERY);
    } else {
      portENTER_CRITICAL(&runtimeMux);
      appRuntime.diagnostics.recoveryFailCount++;
      portEXIT_CRITICAL(&runtimeMux);
    }
  }
}

static void canRuntimeTask(void *arg) {
  (void)arg;
  for (;;) {
    twai_message_t frame = {};
    for (uint16_t count = 0u; count < CAN_RX_BATCH_MAX; ++count) {
      if (twai_receive(&frame, pdMS_TO_TICKS(2)) != ESP_OK) break;
      canRuntimeProcessFrame(frame, millis());
    }
    canRuntimePollRecovery(millis());
    canRuntimeUpdateRestore(millis());
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}
