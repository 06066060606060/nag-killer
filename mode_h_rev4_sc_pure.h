#pragma once

#include <stdint.h>

#include "nag_human_v4_pure.h"

static constexpr uint16_t MODE_H_SC_TORQUE_MIN_RAW = 1870u;
static constexpr uint16_t MODE_H_SC_TORQUE_MAX_RAW = 2230u;

struct NagVisualWarningTrackerPure {
  uint8_t previousVisual;
  bool active;
  uint32_t epoch;
  uint32_t enterMs;
};

static inline bool nagVisualWarningActivePure(uint8_t visual) {
  return visual >= 3u && visual <= 5u;
}

static inline void nagVisualWarningUpdatePure(
    NagVisualWarningTrackerPure &state,
    uint8_t visual,
    uint32_t nowMs) {
  const bool activeNow = nagVisualWarningActivePure(visual);
  if (!state.active && activeNow) {
    state.epoch++;
    if (state.epoch == 0u) state.epoch = 1u;
    state.enterMs = nowMs;
  }
  state.previousVisual = visual;
  state.active = activeNow;
}

static inline uint16_t modeHScClampTorqueRawPure(uint16_t raw) {
  if (raw < MODE_H_SC_TORQUE_MIN_RAW) return MODE_H_SC_TORQUE_MIN_RAW;
  if (raw > MODE_H_SC_TORQUE_MAX_RAW) return MODE_H_SC_TORQUE_MAX_RAW;
  return raw;
}

static inline NagHumanV1StepResultPure modeHRev4ScStepPure(
    NagHumanV4StatePure &state,
    const NagHumanV4ConfigPure &config,
    uint32_t nowMs,
    uint16_t sourceRaw,
    bool runAllowed,
    bool speedValid,
    bool speedFresh,
    uint16_t speedRaw,
    const NagVisualWarningTrackerPure &visual) {
  NagHumanV1StepResultPure result = nagHumanV4StepPure(
      state, config, nowMs, sourceRaw, runAllowed,
      speedValid, speedFresh, speedRaw,
      visual.epoch, visual.enterMs, visual.active);
  if (result.tx) result.raw = modeHScClampTorqueRawPure(result.raw);
  return result;
}
