#pragma once

#include <Arduino.h>
#include <string.h>

#include "config_model_pure.h"
#include "das_source_lock_pure.h"
#include "das_status_pure.h"
#include "nag_restore_pure.h"
#include "nag_safety_gate_pure.h"
#include "party_speed_pure.h"

enum NagRuntimeResetReason : uint8_t {
  RUNTIME_RESET_BOOT = 0,
  RUNTIME_RESET_MASTER,
  RUNTIME_RESET_PROFILE,
  RUNTIME_RESET_MODE,
  RUNTIME_RESET_AP_GATE,
  RUNTIME_RESET_CAN_RECOVERY,
  RUNTIME_RESET_OTA,
  RUNTIME_RESET_FACTORY,
};

struct AppLiveContext {
  bool dasValid;
  uint8_t apState;
  uint8_t visualState;
  uint32_t lastDasMs;
  bool speedValid;
  uint16_t speedRaw;
  int32_t speedKphX100;
  uint32_t lastSpeedMs;
  bool epasValid;
  uint8_t epasData[8];
  uint16_t epasTorqueRaw;
  uint8_t epasHandsOn;
  uint32_t lastEpasMs;
  NagVisualWarningTrackerPure visual;
};

struct AppDiagnostics {
  uint32_t rxFrames;
  uint32_t lastCanFrameMs;
  uint32_t epasRxFrames;
  uint32_t txOk;
  uint32_t txFail;
  uint32_t lastTxMs;
  uint32_t rejectedFrames;
  uint32_t selfEchoFrames;
  uint32_t recoveryCount;
  uint32_t recoveryFailCount;
  uint8_t blockReason;
  uint32_t lastBlockMs;
};

struct AppRuntimeState {
  AppLiveContext live;
  AppDiagnostics diagnostics;
  DasSourceLockPure dasSource;
  NagRestoreStatePure restore;
  NagHumanV4StatePure modeH;
  bool modeHInitialized;
  bool exactEchoValid;
  uint8_t exactEchoData[8];
  uint32_t exactEchoMs;
  uint32_t modeBPhaseStartMs;
  uint32_t modeStepMs;
  uint8_t modeTorqueIndex;
  uint16_t hoSequence;
  uint16_t modeCRaw;
  uint16_t modeCSeed;
  bool effectiveEnabled;
  uint32_t restoreRemainingMs;
  uint8_t restorePhase;
  NagHumanV1StepResultPure lastModeHDecision;
  uint8_t resetReason;
};

static Config cfg = configDefaultsPure();
static portMUX_TYPE cfgMux = portMUX_INITIALIZER_UNLOCKED;
static AppRuntimeState appRuntime = {};
static portMUX_TYPE runtimeMux = portMUX_INITIALIZER_UNLOCKED;
static volatile uint32_t runtimeEpoch = 1u;
static volatile bool twaiReady = false;
static volatile bool twaiRecovering = false;
static volatile bool otaInProgress = false;
static volatile uint32_t canInitTimeMs = 0u;
static SemaphoreHandle_t canTxMutex = nullptr;

static inline bool canTxMutexEnsure() {
  if (canTxMutex) return true;
  canTxMutex = xSemaphoreCreateMutex();
  return canTxMutex != nullptr;
}

static inline Config cfgSnapshot() {
  Config snapshot;
  portENTER_CRITICAL(&cfgMux);
  snapshot = cfg;
  portEXIT_CRITICAL(&cfgMux);
  return snapshot;
}

static inline uint32_t runtimeEpochSnapshot() {
  portENTER_CRITICAL(&runtimeMux);
  const uint32_t snapshot = runtimeEpoch;
  portEXIT_CRITICAL(&runtimeMux);
  return snapshot;
}

static inline bool runtimeEpochStillCurrent(uint32_t expected) {
  return runtimeEpochSnapshot() == expected;
}

static inline void runtimeInvalidate(NagRuntimeResetReason reason) {
  portENTER_CRITICAL(&runtimeMux);
  const AppDiagnostics diagnostics = appRuntime.diagnostics;
  const DasSourceLockPure dasSource = appRuntime.dasSource;
  const NagRestoreStatePure restore =
      nagRestoreStateAfterRuntimeResetPure(appRuntime.restore);
  appRuntime = AppRuntimeState{};
  appRuntime.diagnostics = diagnostics;
  appRuntime.dasSource = dasSource;
  appRuntime.restore = restore;
  appRuntime.resetReason = (uint8_t)reason;
  appRuntime.modeCRaw = 0x8A7u;
  appRuntime.effectiveEnabled = false;
  appRuntime.restoreRemainingMs = restore.ready ? 0u : NAG_RESTORE_DELAY_MS;
  appRuntime.restorePhase = restore.ready
      ? NAG_RESTORE_READY : NAG_RESTORE_OFF;
  appRuntime.diagnostics.blockReason = NAG_BLOCK_RESTORING;
  runtimeEpoch++;
  if (runtimeEpoch == 0u) runtimeEpoch = 1u;
  portEXIT_CRITICAL(&runtimeMux);
}
