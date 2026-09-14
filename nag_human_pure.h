#pragma once
#include <stdint.h>

// Host-testable Mode H event engine. No Arduino/ESP-IDF dependencies.
// Tesla EPAS torsion-bar torque uses raw * 0.01 - 20.5 Nm; raw 2050 is 0 Nm.

static constexpr uint16_t NAG_HUMAN_TORQUE_CENTER_RAW = 2050u;
static constexpr uint16_t NAG_HUMAN_RAW_MAX = 0x0FFFu;
// Mode H primary-event magnitude limits, in 0.01 Nm units.
// This v3.1H1 rebase intentionally caps the H engine at 2.00 Nm.
// The NORMAL preset remains a randomized 1.50–2.00 Nm interaction range.
static constexpr uint16_t NAG_HUMAN_PEAK_ALLOWED_MIN_RAW = 100u; // 1.00 Nm
static constexpr uint16_t NAG_HUMAN_PEAK_ALLOWED_MAX_RAW = 200u; // 2.00 Nm
static constexpr uint16_t NAG_HUMAN_PEAK_DEFAULT_MIN_RAW = 150u; // 1.50 Nm
static constexpr uint16_t NAG_HUMAN_PEAK_DEFAULT_MAX_RAW = 200u; // 2.00 Nm

enum NagHumanPhasePure : uint8_t {
  H_IDLE = 0,
  H_WAIT = 1,
  H_RAMP_IN = 2,
  H_INTERACT = 3,
  H_RAMP_OUT = 4,
  H_REFRACTORY = 5,
  H_PAUSED_STOPPED = 6
};

enum NagHumanEventTypePure : uint8_t {
  H_EVENT_NONE = 0,
  H_EVENT_SIMPLE = 1,
  H_EVENT_CORRECTION = 2
};

enum NagHumanMotionPure : uint8_t {
  H_MOTION_UNKNOWN = 0,
  H_MOTION_STALE = 1,
  H_MOTION_STOPPED = 2,
  H_MOTION_CONFIRMING = 3,
  H_MOTION_MOVING = 4
};

enum NagHumanBlockReasonPure : uint8_t {
  H_BLOCK_NONE = 0,
  H_BLOCK_RUN_GATE = 1,
  H_BLOCK_SPEED_STALE = 2,
  H_BLOCK_STOPPED = 3,
  H_BLOCK_MOVE_CONFIRMING = 4
};

struct NagHumanConfigPure {
  uint16_t waitMinMs;
  uint16_t waitMaxMs;
  uint16_t rampInMinMs;
  uint16_t rampInMaxMs;
  uint16_t interactMinMs;
  uint16_t interactMaxMs;
  uint16_t rampOutMinMs;
  uint16_t rampOutMaxMs;
  uint16_t refractoryMinMs;
  uint16_t refractoryMaxMs;
  uint16_t peakMinRaw;
  uint16_t peakMaxRaw;
  uint16_t correctionMinRaw;
  uint16_t correctionMaxRaw;
  uint8_t directionPersistencePct;
  uint8_t correctionProbabilityPct;
  int16_t stopThresholdKphX100;
  int16_t moveThresholdKphX100;
  uint16_t moveConfirmMs;
};

struct NagHumanEventPure {
  uint32_t waitDurationMs;
  uint32_t rampInDurationMs;
  uint32_t interactDurationMs;
  uint32_t rampOutDurationMs;
  uint32_t refractoryDurationMs;
  uint16_t peakRaw;
  uint16_t correctionRaw;
  int8_t direction;
  uint8_t type;
};

struct NagHumanStatePure {
  uint8_t phase;
  uint8_t motion;
  bool movingConfirmed;
  bool moveCandidateActive;
  bool eventValid;
  bool carrier;
  uint32_t moveCandidateStartMs;
  uint32_t phaseStartMs;
  uint32_t eventStartMs;
  uint32_t sessionStartMs;
  uint32_t rngState;
  int8_t previousDirection;
  uint16_t rampBaselineRaw;
  uint16_t outputRaw;
  NagHumanEventPure event;
  uint32_t eventCount;
  uint32_t simpleCount;
  uint32_t correctionCount;
};

struct NagHumanStepResultPure {
  bool tx;
  bool setHo;
  bool carrier;
  uint16_t raw;
  uint8_t phase;
  uint8_t motion;
  uint8_t blockReason;
};

static inline NagHumanConfigPure nagHumanDefaultConfigPure() {
  NagHumanConfigPure c = {};
  c.waitMinMs = 1200u;
  c.waitMaxMs = 3000u;
  c.rampInMinMs = 260u;
  c.rampInMaxMs = 520u;
  c.interactMinMs = 700u;
  c.interactMaxMs = 1300u;
  c.rampOutMinMs = 320u;
  c.rampOutMaxMs = 680u;
  c.refractoryMinMs = 800u;
  c.refractoryMaxMs = 1800u;
  c.peakMinRaw = NAG_HUMAN_PEAK_DEFAULT_MIN_RAW;
  c.peakMaxRaw = NAG_HUMAN_PEAK_DEFAULT_MAX_RAW;
  c.correctionMinRaw = 55u;
  c.correctionMaxRaw = 95u;
  c.directionPersistencePct = 65u;
  c.correctionProbabilityPct = 30u;
  c.stopThresholdKphX100 = 16;
  c.moveThresholdKphX100 = 64;
  c.moveConfirmMs = 300u;
  return c;
}

static inline bool nagHumanPeakRangeValidPure(uint16_t minRaw, uint16_t maxRaw) {
  return minRaw >= NAG_HUMAN_PEAK_ALLOWED_MIN_RAW &&
         maxRaw <= NAG_HUMAN_PEAK_ALLOWED_MAX_RAW &&
         minRaw <= maxRaw;
}

static inline void nagHumanSanitizePeakRangePure(NagHumanConfigPure &c) {
  if (c.peakMinRaw < NAG_HUMAN_PEAK_ALLOWED_MIN_RAW)
    c.peakMinRaw = NAG_HUMAN_PEAK_ALLOWED_MIN_RAW;
  if (c.peakMinRaw > NAG_HUMAN_PEAK_ALLOWED_MAX_RAW)
    c.peakMinRaw = NAG_HUMAN_PEAK_ALLOWED_MAX_RAW;
  if (c.peakMaxRaw < NAG_HUMAN_PEAK_ALLOWED_MIN_RAW)
    c.peakMaxRaw = NAG_HUMAN_PEAK_ALLOWED_MIN_RAW;
  if (c.peakMaxRaw > NAG_HUMAN_PEAK_ALLOWED_MAX_RAW)
    c.peakMaxRaw = NAG_HUMAN_PEAK_ALLOWED_MAX_RAW;
  if (c.peakMinRaw > c.peakMaxRaw) {
    c.peakMinRaw = NAG_HUMAN_PEAK_DEFAULT_MIN_RAW;
    c.peakMaxRaw = NAG_HUMAN_PEAK_DEFAULT_MAX_RAW;
  }
}

static inline uint32_t nagHumanSanitizeSeedPure(uint32_t seed) {
  return seed == 0u ? 0x6D2B79F5u : seed;
}

static inline uint32_t nagHumanNextRandomPure(uint32_t &state) {
  uint32_t x = nagHumanSanitizeSeedPure(state);
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  state = nagHumanSanitizeSeedPure(x);
  return state;
}

static inline uint32_t nagHumanRangePure(uint32_t &state, uint32_t lo, uint32_t hi) {
  if (hi < lo) {
    const uint32_t t = lo;
    lo = hi;
    hi = t;
  }
  const uint32_t span = hi - lo + 1u;
  return lo + (span == 0u ? 0u : (nagHumanNextRandomPure(state) % span));
}

static inline void nagHumanClearEventPure(NagHumanStatePure &s) {
  s.event = NagHumanEventPure{};
  s.eventValid = false;
  s.carrier = false;
  s.phaseStartMs = 0u;
  s.eventStartMs = 0u;
  s.sessionStartMs = 0u;
  s.rampBaselineRaw = NAG_HUMAN_TORQUE_CENTER_RAW;
  s.outputRaw = NAG_HUMAN_TORQUE_CENTER_RAW;
}

static inline void nagHumanResetRuntimePure(NagHumanStatePure &s, uint8_t phase) {
  nagHumanClearEventPure(s);
  s.phase = phase;
  s.motion = phase == H_PAUSED_STOPPED ? H_MOTION_STOPPED : H_MOTION_UNKNOWN;
  s.movingConfirmed = false;
  s.moveCandidateActive = false;
  s.moveCandidateStartMs = 0u;
  s.previousDirection = 0;
}

static inline void nagHumanInitPure(NagHumanStatePure &s, uint32_t seed) {
  s = NagHumanStatePure{};
  s.rngState = nagHumanSanitizeSeedPure(seed);
  s.phase = H_IDLE;
  s.motion = H_MOTION_UNKNOWN;
  s.rampBaselineRaw = NAG_HUMAN_TORQUE_CENTER_RAW;
  s.outputRaw = NAG_HUMAN_TORQUE_CENTER_RAW;
}

static inline void nagHumanReseedPure(NagHumanStatePure &s, uint32_t seed) {
  const uint32_t eventCount = s.eventCount;
  const uint32_t simpleCount = s.simpleCount;
  const uint32_t correctionCount = s.correctionCount;
  nagHumanInitPure(s, seed);
  s.eventCount = eventCount;
  s.simpleCount = simpleCount;
  s.correctionCount = correctionCount;
}

static inline NagHumanEventPure nagHumanGenerateEventPure(NagHumanStatePure &s,
                                                           const NagHumanConfigPure &c) {
  NagHumanEventPure e = {};
  e.waitDurationMs = nagHumanRangePure(s.rngState, c.waitMinMs, c.waitMaxMs);
  e.rampInDurationMs = nagHumanRangePure(s.rngState, c.rampInMinMs, c.rampInMaxMs);
  e.interactDurationMs = nagHumanRangePure(s.rngState, c.interactMinMs, c.interactMaxMs);
  e.rampOutDurationMs = nagHumanRangePure(s.rngState, c.rampOutMinMs, c.rampOutMaxMs);
  e.refractoryDurationMs = nagHumanRangePure(s.rngState, c.refractoryMinMs, c.refractoryMaxMs);
  e.peakRaw = (uint16_t)nagHumanRangePure(s.rngState, c.peakMinRaw, c.peakMaxRaw);
  e.correctionRaw = (uint16_t)nagHumanRangePure(s.rngState, c.correctionMinRaw, c.correctionMaxRaw);

  if (s.previousDirection == 1 || s.previousDirection == -1) {
    const uint32_t retainRoll = nagHumanRangePure(s.rngState, 0u, 99u);
    e.direction = retainRoll < c.directionPersistencePct
        ? s.previousDirection
        : (int8_t)-s.previousDirection;
  } else {
    e.direction = (nagHumanNextRandomPure(s.rngState) & 1u) ? 1 : -1;
  }
  s.previousDirection = e.direction;

  const uint32_t typeRoll = nagHumanRangePure(s.rngState, 0u, 99u);
  e.type = typeRoll < c.correctionProbabilityPct ? H_EVENT_CORRECTION : H_EVENT_SIMPLE;
  return e;
}

static inline uint32_t nagHumanProgressQ15Pure(uint32_t elapsed, uint32_t duration) {
  if (duration == 0u || elapsed >= duration) return 32768u;
  return (uint32_t)(((uint64_t)elapsed * 32768u) / duration);
}

static inline uint32_t nagHumanSmoothstepQ15Pure(uint32_t x) {
  if (x >= 32768u) return 32768u;
  const uint64_t x2 = ((uint64_t)x * x + 16384u) >> 15;
  const uint32_t threeMinusTwoX = 98304u - (2u * x);
  uint64_t y = (x2 * threeMinusTwoX + 16384u) >> 15;
  if (y > 32768u) y = 32768u;
  return (uint32_t)y;
}

static inline uint16_t nagHumanClampRawPure(int32_t raw) {
  if (raw < 0) return 0u;
  if (raw > (int32_t)NAG_HUMAN_RAW_MAX) return NAG_HUMAN_RAW_MAX;
  return (uint16_t)raw;
}

static inline uint16_t nagHumanLerpRawPure(uint16_t a, uint16_t b, uint32_t q15) {
  if (q15 >= 32768u) return b;
  const int32_t delta = (int32_t)b - (int32_t)a;
  const int64_t scaled = (int64_t)delta * (int64_t)q15;
  const int32_t rounded = scaled >= 0
      ? (int32_t)((scaled + 16384) / 32768)
      : (int32_t)((scaled - 16384) / 32768);
  return nagHumanClampRawPure((int32_t)a + rounded);
}

static inline uint16_t nagHumanPrimaryTargetRawPure(const NagHumanEventPure &e) {
  return nagHumanClampRawPure((int32_t)NAG_HUMAN_TORQUE_CENTER_RAW +
                              (int32_t)e.direction * (int32_t)e.peakRaw);
}

static inline uint16_t nagHumanCorrectionTargetRawPure(const NagHumanEventPure &e) {
  return nagHumanClampRawPure((int32_t)NAG_HUMAN_TORQUE_CENTER_RAW -
                              (int32_t)e.direction * (int32_t)e.correctionRaw);
}

static inline uint16_t nagHumanFinalEventRawPure(const NagHumanEventPure &e) {
  return e.type == H_EVENT_CORRECTION
      ? nagHumanCorrectionTargetRawPure(e)
      : nagHumanPrimaryTargetRawPure(e);
}

static inline void nagHumanPausePure(NagHumanStatePure &s, uint8_t motion) {
  nagHumanClearEventPure(s);
  s.phase = H_PAUSED_STOPPED;
  s.motion = motion;
  s.movingConfirmed = false;
  s.moveCandidateActive = false;
  s.moveCandidateStartMs = 0u;
  s.previousDirection = 0;
}

static inline void nagHumanBeginWaitPure(NagHumanStatePure &s,
                                         const NagHumanConfigPure &c,
                                         uint32_t nowMs,
                                         bool newSession) {
  s.event = nagHumanGenerateEventPure(s, c);
  s.eventValid = true;
  s.phase = H_WAIT;
  s.phaseStartMs = nowMs;
  s.eventStartMs = 0u;
  if (newSession || s.sessionStartMs == 0u) s.sessionStartMs = nowMs;
  s.carrier = true;
}

static inline bool nagHumanMotionAllowsPure(NagHumanStatePure &s,
                                            const NagHumanConfigPure &c,
                                            uint32_t nowMs,
                                            bool speedValid,
                                            bool speedFresh,
                                            uint16_t speedRaw,
                                            uint8_t &blockReason) {
  if (!speedValid || !speedFresh) {
    nagHumanPausePure(s, H_MOTION_STALE);
    blockReason = H_BLOCK_SPEED_STALE;
    return false;
  }

  const int32_t speedKphX100 = (int32_t)speedRaw * 8 - 4000;
  if (s.movingConfirmed) {
    if (speedKphX100 <= c.stopThresholdKphX100) {
      nagHumanPausePure(s, H_MOTION_STOPPED);
      blockReason = H_BLOCK_STOPPED;
      return false;
    }
    s.motion = H_MOTION_MOVING;
    blockReason = H_BLOCK_NONE;
    return true;
  }

  if (speedKphX100 >= c.moveThresholdKphX100) {
    if (!s.moveCandidateActive) {
      s.moveCandidateActive = true;
      s.moveCandidateStartMs = nowMs;
    }
    if (c.moveConfirmMs == 0u || (uint32_t)(nowMs - s.moveCandidateStartMs) >= c.moveConfirmMs) {
      s.movingConfirmed = true;
      s.moveCandidateActive = false;
      s.motion = H_MOTION_MOVING;
      s.phase = H_IDLE;
      blockReason = H_BLOCK_NONE;
      return true;
    }
    s.phase = H_PAUSED_STOPPED;
    s.motion = H_MOTION_CONFIRMING;
    blockReason = H_BLOCK_MOVE_CONFIRMING;
    return false;
  }

  s.moveCandidateActive = false;
  s.moveCandidateStartMs = 0u;
  s.phase = H_PAUSED_STOPPED;
  s.motion = H_MOTION_STOPPED;
  blockReason = H_BLOCK_STOPPED;
  return false;
}

static inline const char* nagHumanPhaseNamePure(uint8_t phase) {
  switch (phase) {
    case H_WAIT: return "WAIT";
    case H_RAMP_IN: return "RAMP_IN";
    case H_INTERACT: return "INTERACT";
    case H_RAMP_OUT: return "RAMP_OUT";
    case H_REFRACTORY: return "REFRACTORY";
    case H_PAUSED_STOPPED: return "PAUSED_STOPPED";
    default: return "IDLE";
  }
}

static inline const char* nagHumanEventTypeNamePure(uint8_t type) {
  if (type == H_EVENT_SIMPLE) return "SIMPLE";
  if (type == H_EVENT_CORRECTION) return "CORRECTION";
  return "NONE";
}

static inline const char* nagHumanMotionNamePure(uint8_t motion) {
  switch (motion) {
    case H_MOTION_STALE: return "STALE";
    case H_MOTION_STOPPED: return "STOPPED";
    case H_MOTION_CONFIRMING: return "CONFIRMING";
    case H_MOTION_MOVING: return "MOVING";
    default: return "UNKNOWN";
  }
}

static inline NagHumanStepResultPure nagHumanStepPure(
    NagHumanStatePure &s,
    const NagHumanConfigPure &c,
    uint32_t nowMs,
    uint16_t sourceRaw,
    bool runAllowed,
    bool speedValid,
    bool speedFresh,
    uint16_t speedRaw) {
  NagHumanStepResultPure result = {};
  result.raw = sourceRaw;
  result.phase = s.phase;
  result.motion = s.motion;

  if (!runAllowed) {
    nagHumanResetRuntimePure(s, H_IDLE);
    result.phase = s.phase;
    result.motion = s.motion;
    result.blockReason = H_BLOCK_RUN_GATE;
    return result;
  }

  uint8_t motionBlock = H_BLOCK_NONE;
  if (!nagHumanMotionAllowsPure(s, c, nowMs, speedValid, speedFresh, speedRaw, motionBlock)) {
    result.phase = s.phase;
    result.motion = s.motion;
    result.blockReason = motionBlock;
    return result;
  }

  if (s.phase == H_IDLE || s.phase == H_PAUSED_STOPPED || !s.eventValid) {
    nagHumanBeginWaitPure(s, c, nowMs, true);
  }

  // Catch up across phase boundaries without accumulating scheduler drift.
  for (uint8_t transitions = 0; transitions < 8u; transitions++) {
    const uint32_t elapsed = (uint32_t)(nowMs - s.phaseStartMs);
    uint32_t duration = 0u;
    switch (s.phase) {
      case H_WAIT: duration = s.event.waitDurationMs; break;
      case H_RAMP_IN: duration = s.event.rampInDurationMs; break;
      case H_INTERACT: duration = s.event.interactDurationMs; break;
      case H_RAMP_OUT: duration = s.event.rampOutDurationMs; break;
      case H_REFRACTORY: duration = s.event.refractoryDurationMs; break;
      default: duration = 0u; break;
    }
    if (duration == 0u || elapsed < duration) break;

    s.phaseStartMs += duration;
    if (s.phase == H_WAIT) {
      s.phase = H_RAMP_IN;
      s.eventStartMs = s.phaseStartMs;
      s.rampBaselineRaw = sourceRaw;
      s.eventCount++;
      if (s.event.type == H_EVENT_CORRECTION) s.correctionCount++;
      else s.simpleCount++;
    } else if (s.phase == H_RAMP_IN) {
      s.phase = H_INTERACT;
    } else if (s.phase == H_INTERACT) {
      s.phase = H_RAMP_OUT;
    } else if (s.phase == H_RAMP_OUT) {
      s.phase = H_REFRACTORY;
    } else if (s.phase == H_REFRACTORY) {
      nagHumanBeginWaitPure(s, c, s.phaseStartMs, false);
    } else {
      break;
    }
  }

  const uint32_t elapsed = (uint32_t)(nowMs - s.phaseStartMs);
  uint16_t raw = sourceRaw;
  bool carrier = false;

  if (s.phase == H_WAIT || s.phase == H_REFRACTORY) {
    raw = sourceRaw;
    carrier = true;
  } else if (s.phase == H_RAMP_IN) {
    const uint32_t p = nagHumanSmoothstepQ15Pure(
        nagHumanProgressQ15Pure(elapsed, s.event.rampInDurationMs));
    raw = nagHumanLerpRawPure(s.rampBaselineRaw, nagHumanPrimaryTargetRawPure(s.event), p);
  } else if (s.phase == H_INTERACT) {
    const uint16_t primary = nagHumanPrimaryTargetRawPure(s.event);
    if (s.event.type != H_EVENT_CORRECTION) {
      raw = primary;
    } else {
      const uint32_t correctionStart = (s.event.interactDurationMs * 65u) / 100u;
      if (elapsed <= correctionStart || correctionStart >= s.event.interactDurationMs) {
        raw = primary;
      } else {
        const uint32_t correctionElapsed = elapsed - correctionStart;
        const uint32_t correctionDuration = s.event.interactDurationMs - correctionStart;
        const uint32_t p = nagHumanSmoothstepQ15Pure(
            nagHumanProgressQ15Pure(correctionElapsed, correctionDuration));
        raw = nagHumanLerpRawPure(primary, nagHumanCorrectionTargetRawPure(s.event), p);
      }
    }
  } else if (s.phase == H_RAMP_OUT) {
    const uint32_t p = nagHumanSmoothstepQ15Pure(
        nagHumanProgressQ15Pure(elapsed, s.event.rampOutDurationMs));
    raw = nagHumanLerpRawPure(nagHumanFinalEventRawPure(s.event), sourceRaw, p);
  }

  s.outputRaw = raw;
  s.carrier = carrier;
  result.tx = true;
  result.setHo = true;
  result.carrier = carrier;
  result.raw = raw;
  result.phase = s.phase;
  result.motion = s.motion;
  result.blockReason = H_BLOCK_NONE;
  return result;
}
