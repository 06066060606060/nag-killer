#pragma once

#include <stdint.h>

static constexpr uint32_t NAG_RESTORE_DELAY_MS = 3000u;

enum NagRestorePhasePure : uint8_t {
  NAG_RESTORE_OFF = 0,
  NAG_RESTORE_WAITING,
  NAG_RESTORE_COUNTDOWN,
  NAG_RESTORE_READY,
};

struct NagRestoreStatePure {
  bool timing;
  bool ready;
  uint32_t startMs;
};

struct NagRestoreResultPure {
  bool effectiveEnabled;
  uint32_t remainingMs;
  uint8_t phase;
};

struct NagRestorePrerequisitesInputPure {
  bool profileSelected;
  bool twaiReady;
  bool otaActive;
  bool epasValid;
  bool stockHandsOnSupported;
  bool dasRequired;
  bool dasFresh;
  bool speedRequired;
  bool speedFresh;
};

static inline NagRestoreStatePure nagRestoreStateAfterRuntimeResetPure(
    const NagRestoreStatePure &state) {
  return {false, state.ready, 0u};
}

static inline bool nagRestorePrerequisitesReadyPure(
    const NagRestorePrerequisitesInputPure &input) {
  if (!input.profileSelected || !input.twaiReady || input.otaActive ||
      !input.epasValid || !input.stockHandsOnSupported) return false;
  if (input.dasRequired && !input.dasFresh) return false;
  if (input.speedRequired && !input.speedFresh) return false;
  return true;
}

static inline NagRestoreResultPure nagRestoreUpdatePure(
    NagRestoreStatePure &state,
    uint32_t nowMs,
    bool requestedOn,
    bool prerequisitesReady,
    uint32_t delayMs = NAG_RESTORE_DELAY_MS) {
  if (!requestedOn) {
    state.timing = false;
    state.startMs = 0u;
    return {false, 0u, NAG_RESTORE_OFF};
  }
  if (state.ready)
    return {true, 0u, NAG_RESTORE_READY};
  if (!prerequisitesReady) {
    state.timing = false;
    state.startMs = 0u;
    return {false, delayMs, NAG_RESTORE_WAITING};
  }
  if (!state.timing) {
    state.timing = true;
    state.startMs = nowMs;
  }
  const uint32_t elapsed = (uint32_t)(nowMs - state.startMs);
  if (elapsed >= delayMs) {
    state.ready = true;
    return {true, 0u, NAG_RESTORE_READY};
  }
  return {false, delayMs - elapsed, NAG_RESTORE_COUNTDOWN};
}
