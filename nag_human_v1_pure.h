#pragma once
#include <stdint.h>

// Host-testable Mode H event engine. No Arduino/ESP-IDF dependencies.
// Tesla EPAS torsion-bar torque uses raw * 0.01 - 20.5 Nm; raw 2050 is 0 Nm.

static constexpr uint16_t NAG_HUMAN_V1_TORQUE_CENTER_RAW = 2050u;
static constexpr uint16_t NAG_HUMAN_V1_RAW_MAX = 0x0FFFu;
// Mode H LAB-editable primary-event magnitude limits, in 0.01 Nm units.
// The lower bound stays above the fixed correction ceiling (0.95 Nm).
// LAB may extend the primary Human Interaction peak up to 3.00 Nm; the
// production Rev.1 Plus default is 1.50–2.20 Nm.
static constexpr uint16_t NAG_HUMAN_V1_PEAK_ALLOWED_MIN_RAW = 100u; // 1.00 Nm
static constexpr uint16_t NAG_HUMAN_V1_PEAK_ALLOWED_MAX_RAW = 300u; // 3.00 Nm
static constexpr uint16_t NAG_HUMAN_V1_REV1_PEAK_MIN_RAW = 150u; // 1.50 Nm
static constexpr uint16_t NAG_HUMAN_V1_REV1_PEAK_MAX_RAW = 200u; // 2.00 Nm
static constexpr uint16_t NAG_HUMAN_V1_B19_DEFAULT_MIN_RAW = 180u; // b19 untouched default
static constexpr uint16_t NAG_HUMAN_V1_B19_DEFAULT_MAX_RAW = 240u; // b19 untouched default
static constexpr uint16_t NAG_HUMAN_V1_PEAK_DEFAULT_MIN_RAW = 150u; // Rev.1 Plus 1.50 Nm
static constexpr uint16_t NAG_HUMAN_V1_PEAK_DEFAULT_MAX_RAW = 220u; // Rev.1 Plus 2.20 Nm
static constexpr uint16_t NAG_HUMAN_V1_WAIT_ALLOWED_MIN_MS = 300u;
static constexpr uint16_t NAG_HUMAN_V1_WAIT_ALLOWED_MAX_MS = 5000u;
static constexpr uint16_t NAG_HUMAN_V1_REFRACTORY_ALLOWED_MIN_MS = 300u;
static constexpr uint16_t NAG_HUMAN_V1_REFRACTORY_ALLOWED_MAX_MS = 2500u;
static constexpr uint16_t NAG_HUMAN_V1_HO_HOLD_MIN_MS = 150u;
static constexpr uint16_t NAG_HUMAN_V1_HO_HOLD_MAX_MS = 250u;

enum NagHumanV1PhasePure : uint8_t {
  H1_IDLE = 0,
  H1_WAIT = 1,
  H1_RAMP_IN = 2,
  H1_INTERACT = 3,
  H1_RAMP_OUT = 4,
  H1_REFRACTORY = 5,
  H1_PAUSED_STOPPED = 6
};

enum NagHumanV1EventTypePure : uint8_t {
  H1_EVENT_NONE = 0,
  H1_EVENT_SIMPLE = 1,
  H1_EVENT_CORRECTION = 2
};

enum NagHumanV1MotionPure : uint8_t {
  H1_MOTION_UNKNOWN = 0,
  H1_MOTION_STALE = 1,
  H1_MOTION_STOPPED = 2,
  H1_MOTION_CONFIRMING = 3,
  H1_MOTION_MOVING = 4
};

enum NagHumanV1BlockReasonPure : uint8_t {
  H1_BLOCK_NONE = 0,
  H1_BLOCK_RUN_GATE = 1,
  H1_BLOCK_SPEED_STALE = 2,
  H1_BLOCK_STOPPED = 3,
  H1_BLOCK_MOVE_CONFIRMING = 4
};

struct NagHumanV1ConfigPure {
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
  uint8_t hoOverridePct;
  int16_t stopThresholdKphX100;
  int16_t moveThresholdKphX100;
  uint16_t moveConfirmMs;
};

struct NagHumanV1EventPure {
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

struct NagHumanV1StatePure {
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
  NagHumanV1EventPure event;
  uint32_t eventCount;
  uint32_t simpleCount;
  uint32_t correctionCount;
  bool hoDecisionValid;
  bool hoOverrideActive;
  uint32_t hoDecisionStartMs;
  uint16_t hoDecisionDurationMs;
};

struct NagHumanV1StepResultPure {
  bool tx;
  bool setHo;
  bool hoOverrideValid;
  uint8_t hoLevel;
  bool carrier;
  uint16_t raw;
  uint8_t phase;
  uint8_t motion;
  uint8_t blockReason;
};

static inline void nagHumanV1MigrateB19DefaultToRev1PlusPure(uint16_t &minRaw, uint16_t &maxRaw) {
  if (minRaw == NAG_HUMAN_V1_B19_DEFAULT_MIN_RAW &&
      maxRaw == NAG_HUMAN_V1_B19_DEFAULT_MAX_RAW) {
    minRaw = NAG_HUMAN_V1_PEAK_DEFAULT_MIN_RAW;
    maxRaw = NAG_HUMAN_V1_PEAK_DEFAULT_MAX_RAW;
  }
}

static inline NagHumanV1ConfigPure nagHumanV1Rev1PlusConfigPure() {
  NagHumanV1ConfigPure c = {};
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
  c.peakMinRaw = NAG_HUMAN_V1_PEAK_DEFAULT_MIN_RAW;
  c.peakMaxRaw = NAG_HUMAN_V1_PEAK_DEFAULT_MAX_RAW;
  c.correctionMinRaw = 55u;
  c.correctionMaxRaw = 95u;
  c.directionPersistencePct = 65u;
  c.correctionProbabilityPct = 30u;
  c.hoOverridePct = 100u;
  c.stopThresholdKphX100 = 16;
  c.moveThresholdKphX100 = 64;
  c.moveConfirmMs = 300u;
  return c;
}

static inline NagHumanV1ConfigPure nagHumanV1Rev1ConfigPure() {
  NagHumanV1ConfigPure c = nagHumanV1Rev1PlusConfigPure();
  c.peakMinRaw = NAG_HUMAN_V1_REV1_PEAK_MIN_RAW;
  c.peakMaxRaw = NAG_HUMAN_V1_REV1_PEAK_MAX_RAW;
  return c;
}



static inline bool nagHumanV1TimingValidPure(uint16_t waitMinMs, uint16_t waitMaxMs,
                                            uint16_t refractoryMinMs, uint16_t refractoryMaxMs) {
  return waitMinMs >= NAG_HUMAN_V1_WAIT_ALLOWED_MIN_MS &&
         waitMaxMs <= NAG_HUMAN_V1_WAIT_ALLOWED_MAX_MS &&
         waitMinMs <= waitMaxMs &&
         refractoryMinMs >= NAG_HUMAN_V1_REFRACTORY_ALLOWED_MIN_MS &&
         refractoryMaxMs <= NAG_HUMAN_V1_REFRACTORY_ALLOWED_MAX_MS &&
         refractoryMinMs <= refractoryMaxMs;
}

static inline bool nagHumanV1PeakRangeValidPure(uint16_t minRaw, uint16_t maxRaw) {
  return minRaw >= NAG_HUMAN_V1_PEAK_ALLOWED_MIN_RAW &&
         maxRaw <= NAG_HUMAN_V1_PEAK_ALLOWED_MAX_RAW &&
         minRaw <= maxRaw;
}


static inline uint32_t nagHumanV1SanitizeSeedPure(uint32_t seed) {
  return seed == 0u ? 0x6D2B79F5u : seed;
}

static inline uint32_t nagHumanV1NextRandomPure(uint32_t &state) {
  uint32_t x = nagHumanV1SanitizeSeedPure(state);
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  state = nagHumanV1SanitizeSeedPure(x);
  return state;
}

static inline uint32_t nagHumanV1RangePure(uint32_t &state, uint32_t lo, uint32_t hi) {
  if (hi < lo) {
    const uint32_t t = lo;
    lo = hi;
    hi = t;
  }
  const uint32_t span = hi - lo + 1u;
  return lo + (span == 0u ? 0u : (nagHumanV1NextRandomPure(state) % span));
}

static inline void nagHumanV1ClearEventPure(NagHumanV1StatePure &s) {
  s.event = NagHumanV1EventPure{};
  s.eventValid = false;
  s.carrier = false;
  s.phaseStartMs = 0u;
  s.eventStartMs = 0u;
  s.sessionStartMs = 0u;
  s.rampBaselineRaw = NAG_HUMAN_V1_TORQUE_CENTER_RAW;
  s.outputRaw = NAG_HUMAN_V1_TORQUE_CENTER_RAW;
  s.hoDecisionValid = false;
  s.hoOverrideActive = false;
  s.hoDecisionStartMs = 0u;
  s.hoDecisionDurationMs = 0u;
}

static inline void nagHumanV1ResetRuntimePure(NagHumanV1StatePure &s, uint8_t phase) {
  nagHumanV1ClearEventPure(s);
  s.phase = phase;
  s.motion = phase == H1_PAUSED_STOPPED ? H1_MOTION_STOPPED : H1_MOTION_UNKNOWN;
  s.movingConfirmed = false;
  s.moveCandidateActive = false;
  s.moveCandidateStartMs = 0u;
  s.previousDirection = 0;
}

static inline void nagHumanV1InitPure(NagHumanV1StatePure &s, uint32_t seed) {
  s = NagHumanV1StatePure{};
  s.rngState = nagHumanV1SanitizeSeedPure(seed);
  s.phase = H1_IDLE;
  s.motion = H1_MOTION_UNKNOWN;
  s.rampBaselineRaw = NAG_HUMAN_V1_TORQUE_CENTER_RAW;
  s.outputRaw = NAG_HUMAN_V1_TORQUE_CENTER_RAW;
}

static inline void nagHumanV1ReseedPure(NagHumanV1StatePure &s, uint32_t seed) {
  const uint32_t eventCount = s.eventCount;
  const uint32_t simpleCount = s.simpleCount;
  const uint32_t correctionCount = s.correctionCount;
  nagHumanV1InitPure(s, seed);
  s.eventCount = eventCount;
  s.simpleCount = simpleCount;
  s.correctionCount = correctionCount;
}

static inline NagHumanV1EventPure nagHumanV1GenerateEventPure(NagHumanV1StatePure &s,
                                                           const NagHumanV1ConfigPure &c) {
  NagHumanV1EventPure e = {};
  e.waitDurationMs = nagHumanV1RangePure(s.rngState, c.waitMinMs, c.waitMaxMs);
  e.rampInDurationMs = nagHumanV1RangePure(s.rngState, c.rampInMinMs, c.rampInMaxMs);
  e.interactDurationMs = nagHumanV1RangePure(s.rngState, c.interactMinMs, c.interactMaxMs);
  e.rampOutDurationMs = nagHumanV1RangePure(s.rngState, c.rampOutMinMs, c.rampOutMaxMs);
  e.refractoryDurationMs = nagHumanV1RangePure(s.rngState, c.refractoryMinMs, c.refractoryMaxMs);
  e.peakRaw = (uint16_t)nagHumanV1RangePure(s.rngState, c.peakMinRaw, c.peakMaxRaw);
  e.correctionRaw = (uint16_t)nagHumanV1RangePure(s.rngState, c.correctionMinRaw, c.correctionMaxRaw);

  if (s.previousDirection == 1 || s.previousDirection == -1) {
    const uint32_t retainRoll = nagHumanV1RangePure(s.rngState, 0u, 99u);
    e.direction = retainRoll < c.directionPersistencePct
        ? s.previousDirection
        : (int8_t)-s.previousDirection;
  } else {
    e.direction = (nagHumanV1NextRandomPure(s.rngState) & 1u) ? 1 : -1;
  }
  s.previousDirection = e.direction;

  const uint32_t typeRoll = nagHumanV1RangePure(s.rngState, 0u, 99u);
  e.type = typeRoll < c.correctionProbabilityPct ? H1_EVENT_CORRECTION : H1_EVENT_SIMPLE;
  return e;
}

static inline uint32_t nagHumanV1ProgressQ15Pure(uint32_t elapsed, uint32_t duration) {
  if (duration == 0u || elapsed >= duration) return 32768u;
  return (uint32_t)(((uint64_t)elapsed * 32768u) / duration);
}

static inline uint32_t nagHumanV1SmoothstepQ15Pure(uint32_t x) {
  if (x >= 32768u) return 32768u;
  const uint64_t x2 = ((uint64_t)x * x + 16384u) >> 15;
  const uint32_t threeMinusTwoX = 98304u - (2u * x);
  uint64_t y = (x2 * threeMinusTwoX + 16384u) >> 15;
  if (y > 32768u) y = 32768u;
  return (uint32_t)y;
}

static inline uint16_t nagHumanV1ClampRawPure(int32_t raw) {
  if (raw < 0) return 0u;
  if (raw > (int32_t)NAG_HUMAN_V1_RAW_MAX) return NAG_HUMAN_V1_RAW_MAX;
  return (uint16_t)raw;
}

static inline uint16_t nagHumanV1LerpRawPure(uint16_t a, uint16_t b, uint32_t q15) {
  if (q15 >= 32768u) return b;
  const int32_t delta = (int32_t)b - (int32_t)a;
  const int64_t scaled = (int64_t)delta * (int64_t)q15;
  const int32_t rounded = scaled >= 0
      ? (int32_t)((scaled + 16384) / 32768)
      : (int32_t)((scaled - 16384) / 32768);
  return nagHumanV1ClampRawPure((int32_t)a + rounded);
}

static inline uint16_t nagHumanV1PrimaryTargetRawPure(const NagHumanV1EventPure &e) {
  return nagHumanV1ClampRawPure((int32_t)NAG_HUMAN_V1_TORQUE_CENTER_RAW +
                              (int32_t)e.direction * (int32_t)e.peakRaw);
}

static inline uint16_t nagHumanV1CorrectionTargetRawPure(const NagHumanV1EventPure &e) {
  return nagHumanV1ClampRawPure((int32_t)NAG_HUMAN_V1_TORQUE_CENTER_RAW -
                              (int32_t)e.direction * (int32_t)e.correctionRaw);
}

static inline uint16_t nagHumanV1FinalEventRawPure(const NagHumanV1EventPure &e) {
  return e.type == H1_EVENT_CORRECTION
      ? nagHumanV1CorrectionTargetRawPure(e)
      : nagHumanV1PrimaryTargetRawPure(e);
}

static inline void nagHumanV1PausePure(NagHumanV1StatePure &s, uint8_t motion) {
  nagHumanV1ClearEventPure(s);
  s.phase = H1_PAUSED_STOPPED;
  s.motion = motion;
  s.movingConfirmed = false;
  s.moveCandidateActive = false;
  s.moveCandidateStartMs = 0u;
  s.previousDirection = 0;
}

static inline void nagHumanV1BeginWaitPure(NagHumanV1StatePure &s,
                                         const NagHumanV1ConfigPure &c,
                                         uint32_t nowMs,
                                         bool newSession) {
  s.event = nagHumanV1GenerateEventPure(s, c);
  s.eventValid = true;
  s.phase = H1_WAIT;
  s.phaseStartMs = nowMs;
  s.eventStartMs = 0u;
  if (newSession || s.sessionStartMs == 0u) s.sessionStartMs = nowMs;
  s.carrier = true;
}

static inline bool nagHumanV1MotionAllowsPure(NagHumanV1StatePure &s,
                                            const NagHumanV1ConfigPure &c,
                                            uint32_t nowMs,
                                            bool speedValid,
                                            bool speedFresh,
                                            uint16_t speedRaw,
                                            uint8_t &blockReason) {
  if (!speedValid || !speedFresh) {
    nagHumanV1PausePure(s, H1_MOTION_STALE);
    blockReason = H1_BLOCK_SPEED_STALE;
    return false;
  }

  const int32_t speedKphX100 = (int32_t)speedRaw * 8 - 4000;
  if (s.movingConfirmed) {
    if (speedKphX100 <= c.stopThresholdKphX100) {
      nagHumanV1PausePure(s, H1_MOTION_STOPPED);
      blockReason = H1_BLOCK_STOPPED;
      return false;
    }
    s.motion = H1_MOTION_MOVING;
    blockReason = H1_BLOCK_NONE;
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
      s.motion = H1_MOTION_MOVING;
      s.phase = H1_IDLE;
      blockReason = H1_BLOCK_NONE;
      return true;
    }
    s.phase = H1_PAUSED_STOPPED;
    s.motion = H1_MOTION_CONFIRMING;
    blockReason = H1_BLOCK_MOVE_CONFIRMING;
    return false;
  }

  s.moveCandidateActive = false;
  s.moveCandidateStartMs = 0u;
  s.phase = H1_PAUSED_STOPPED;
  s.motion = H1_MOTION_STOPPED;
  blockReason = H1_BLOCK_STOPPED;
  return false;
}

static inline bool nagHumanV1HoOverridePure(NagHumanV1StatePure &s,
                                             const NagHumanV1ConfigPure &c,
                                             uint32_t nowMs) {
  if (c.hoOverridePct >= 100u) {
    s.hoDecisionValid = true;
    s.hoOverrideActive = true;
    s.hoDecisionStartMs = nowMs;
    s.hoDecisionDurationMs = NAG_HUMAN_V1_HO_HOLD_MAX_MS;
    return true;
  }
  if (c.hoOverridePct == 0u) {
    s.hoDecisionValid = true;
    s.hoOverrideActive = false;
    s.hoDecisionStartMs = nowMs;
    s.hoDecisionDurationMs = NAG_HUMAN_V1_HO_HOLD_MAX_MS;
    return false;
  }
  const bool expired = !s.hoDecisionValid ||
      (uint32_t)(nowMs - s.hoDecisionStartMs) >= s.hoDecisionDurationMs;
  if (expired) {
    const uint32_t roll = nagHumanV1RangePure(s.rngState, 0u, 99u);
    s.hoOverrideActive = roll < c.hoOverridePct;
    s.hoDecisionDurationMs = (uint16_t)nagHumanV1RangePure(
        s.rngState, NAG_HUMAN_V1_HO_HOLD_MIN_MS, NAG_HUMAN_V1_HO_HOLD_MAX_MS);
    s.hoDecisionStartMs = nowMs;
    s.hoDecisionValid = true;
  }
  return s.hoOverrideActive;
}

static inline const char* nagHumanV1PhaseNamePure(uint8_t phase) {
  switch (phase) {
    case H1_WAIT: return "WAIT";
    case H1_RAMP_IN: return "RAMP_IN";
    case H1_INTERACT: return "INTERACT";
    case H1_RAMP_OUT: return "RAMP_OUT";
    case H1_REFRACTORY: return "REFRACTORY";
    case H1_PAUSED_STOPPED: return "PAUSED_STOPPED";
    default: return "IDLE";
  }
}

static inline const char* nagHumanV1EventTypeNamePure(uint8_t type) {
  if (type == H1_EVENT_SIMPLE) return "SIMPLE";
  if (type == H1_EVENT_CORRECTION) return "CORRECTION";
  return "NONE";
}

static inline const char* nagHumanV1MotionNamePure(uint8_t motion) {
  switch (motion) {
    case H1_MOTION_STALE: return "STALE";
    case H1_MOTION_STOPPED: return "STOPPED";
    case H1_MOTION_CONFIRMING: return "CONFIRMING";
    case H1_MOTION_MOVING: return "MOVING";
    default: return "UNKNOWN";
  }
}

static inline NagHumanV1StepResultPure nagHumanV1StepPure(
    NagHumanV1StatePure &s,
    const NagHumanV1ConfigPure &c,
    uint32_t nowMs,
    uint16_t sourceRaw,
    bool runAllowed,
    bool speedValid,
    bool speedFresh,
    uint16_t speedRaw) {
  NagHumanV1StepResultPure result = {};
  result.raw = sourceRaw;
  result.phase = s.phase;
  result.motion = s.motion;

  if (!runAllowed) {
    nagHumanV1ResetRuntimePure(s, H1_IDLE);
    result.phase = s.phase;
    result.motion = s.motion;
    result.blockReason = H1_BLOCK_RUN_GATE;
    return result;
  }

  uint8_t motionBlock = H1_BLOCK_NONE;
  if (!nagHumanV1MotionAllowsPure(s, c, nowMs, speedValid, speedFresh, speedRaw, motionBlock)) {
    result.phase = s.phase;
    result.motion = s.motion;
    result.blockReason = motionBlock;
    return result;
  }

  if (s.phase == H1_IDLE || s.phase == H1_PAUSED_STOPPED || !s.eventValid) {
    nagHumanV1BeginWaitPure(s, c, nowMs, true);
  }

  // Catch up across phase boundaries without accumulating scheduler drift.
  for (uint8_t transitions = 0; transitions < 8u; transitions++) {
    const uint32_t elapsed = (uint32_t)(nowMs - s.phaseStartMs);
    uint32_t duration = 0u;
    switch (s.phase) {
      case H1_WAIT: duration = s.event.waitDurationMs; break;
      case H1_RAMP_IN: duration = s.event.rampInDurationMs; break;
      case H1_INTERACT: duration = s.event.interactDurationMs; break;
      case H1_RAMP_OUT: duration = s.event.rampOutDurationMs; break;
      case H1_REFRACTORY: duration = s.event.refractoryDurationMs; break;
      default: duration = 0u; break;
    }
    if (duration == 0u || elapsed < duration) break;

    s.phaseStartMs += duration;
    if (s.phase == H1_WAIT) {
      s.phase = H1_RAMP_IN;
      s.eventStartMs = s.phaseStartMs;
      s.rampBaselineRaw = sourceRaw;
      s.eventCount++;
      if (s.event.type == H1_EVENT_CORRECTION) s.correctionCount++;
      else s.simpleCount++;
    } else if (s.phase == H1_RAMP_IN) {
      s.phase = H1_INTERACT;
    } else if (s.phase == H1_INTERACT) {
      s.phase = H1_RAMP_OUT;
    } else if (s.phase == H1_RAMP_OUT) {
      s.phase = H1_REFRACTORY;
    } else if (s.phase == H1_REFRACTORY) {
      nagHumanV1BeginWaitPure(s, c, s.phaseStartMs, false);
    } else {
      break;
    }
  }

  const uint32_t elapsed = (uint32_t)(nowMs - s.phaseStartMs);
  uint16_t raw = sourceRaw;
  bool carrier = false;

  if (s.phase == H1_WAIT || s.phase == H1_REFRACTORY) {
    raw = sourceRaw;
    carrier = true;
  } else if (s.phase == H1_RAMP_IN) {
    const uint32_t p = nagHumanV1SmoothstepQ15Pure(
        nagHumanV1ProgressQ15Pure(elapsed, s.event.rampInDurationMs));
    raw = nagHumanV1LerpRawPure(s.rampBaselineRaw, nagHumanV1PrimaryTargetRawPure(s.event), p);
  } else if (s.phase == H1_INTERACT) {
    const uint16_t primary = nagHumanV1PrimaryTargetRawPure(s.event);
    if (s.event.type != H1_EVENT_CORRECTION) {
      raw = primary;
    } else {
      const uint32_t correctionStart = (s.event.interactDurationMs * 65u) / 100u;
      if (elapsed <= correctionStart || correctionStart >= s.event.interactDurationMs) {
        raw = primary;
      } else {
        const uint32_t correctionElapsed = elapsed - correctionStart;
        const uint32_t correctionDuration = s.event.interactDurationMs - correctionStart;
        const uint32_t p = nagHumanV1SmoothstepQ15Pure(
            nagHumanV1ProgressQ15Pure(correctionElapsed, correctionDuration));
        raw = nagHumanV1LerpRawPure(primary, nagHumanV1CorrectionTargetRawPure(s.event), p);
      }
    }
  } else if (s.phase == H1_RAMP_OUT) {
    const uint32_t p = nagHumanV1SmoothstepQ15Pure(
        nagHumanV1ProgressQ15Pure(elapsed, s.event.rampOutDurationMs));
    raw = nagHumanV1LerpRawPure(nagHumanV1FinalEventRawPure(s.event), sourceRaw, p);
  }

  s.outputRaw = raw;
  s.carrier = carrier;
  result.tx = true;
  result.setHo = nagHumanV1HoOverridePure(s, c, nowMs);
  result.hoOverrideValid = result.setHo;
  result.hoLevel = result.setHo ? 1u : 0u;
  result.carrier = carrier;
  result.raw = raw;
  result.phase = s.phase;
  result.motion = s.motion;
  result.blockReason = H1_BLOCK_NONE;
  return result;
}
