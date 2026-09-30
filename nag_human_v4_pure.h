#pragma once
#include <stdint.h>
#include "nag_human_v3_pure.h"

// Mode H Rev.4: Human Interaction primary events with a per-stock-RX,
// stock-opposite carrier during WAIT and REFRACTORY.
// Torque uses 0.01 Nm raw units around the EPAS center (2050 = 0 Nm).
//
// Carrier rule (outside the near-zero deadband):
//   stock > 0 -> TX = -(abs(stock) + randomCarrier)
//   stock < 0 -> TX = +(abs(stock) + randomCarrier)
// The carrier magnitude is re-sampled on every eligible stock 0x370 RX.
// Primary-event direction is independently biased 80% negative / 20% positive.

static constexpr uint16_t NAG_HUMAN_V4_CARRIER_MIN_ALLOWED_RAW = 10u; // 0.10 Nm
static constexpr uint16_t NAG_HUMAN_V4_CARRIER_MAX_ALLOWED_RAW = 80u; // 0.80 Nm
static constexpr uint16_t NAG_HUMAN_V4_STOCK_DEADBAND_RAW = 5u;       // ±0.05 Nm -> stock passthrough
static constexpr uint16_t NAG_HUMAN_V4_VISUAL_RESCUE_MAX_DELAY_MS = 2000u;
static constexpr uint8_t NAG_HUMAN_V4_NEGATIVE_BIAS_PCT = 80u;

static inline bool nagHumanV4VisualWarningActivePure(uint8_t handsOnState) {
  return handsOnState >= 3u && handsOnState <= 5u;
}

static inline bool nagHumanV4VisualWarningEdgePure(
    uint8_t previousHandsOnState, uint8_t handsOnState) {
  return previousHandsOnState < 3u &&
         nagHumanV4VisualWarningActivePure(handsOnState);
}

struct NagHumanV4ConfigPure {
  NagHumanV1ConfigPure base;
  uint16_t carrierMinRaw;
  uint16_t carrierMaxRaw;
  uint8_t hoPolicy;
  uint16_t ho1ThresholdRaw;
  uint16_t ho2ThresholdRaw;
  bool visualRescueEnabled;
  uint16_t visualRescueDelayMs;
};

struct NagHumanV4StatePure {
  NagHumanV1StatePure base;
  uint32_t carrierRngState;
  uint16_t lastCarrierMagnitudeRaw;
  bool lastCarrierApplied;
  uint16_t rampOutTargetRaw;
  bool rampOutTargetValid;
  uint32_t visualWarningEpochSeen;
  uint32_t visualRescuePendingSinceMs;
  uint32_t visualRescueCount;
  bool visualRescuePending;
};

static inline NagHumanV4ConfigPure nagHumanV4DefaultConfigPure() {
  NagHumanV4ConfigPure c = {};
  // Preserve the established Human Interaction ramp/interact/correction shape,
  // but Rev.4 owns its primary envelope and cadence.
  c.base = nagHumanV1Rev1PlusConfigPure();
  c.base.peakMinRaw = 180u;              // 1.80 Nm
  c.base.peakMaxRaw = 260u;              // 2.60 Nm
  c.base.waitMinMs = 900u;               // 0.90 s
  c.base.waitMaxMs = 3000u;              // 3.00 s
  c.base.refractoryMinMs = 500u;         // 0.50 s
  c.base.refractoryMaxMs = 1500u;        // 1.50 s
  c.base.directionPersistencePct = 0u;   // Rev.4 uses fixed 80/20 direction bias.
  c.base.hoOverridePct = 0u;             // Rev.4 owns the HO decision below.
  c.carrierMinRaw = 10u;                 // 0.10 Nm
  c.carrierMaxRaw = 60u;                 // 0.60 Nm
  c.hoPolicy = H3_HO_TIERED_1_2;
  c.ho1ThresholdRaw = 40u;               // 0.40 Nm
  c.ho2ThresholdRaw = 200u;              // 2.00 Nm
  c.visualRescueEnabled = true;
  c.visualRescueDelayMs = 500u;
  return c;
}

static inline bool nagHumanV4MigrateV16DefaultPure(NagHumanV4ConfigPure &c) {
  const bool untouchedV16Default =
      c.base.peakMinRaw == 150u && c.base.peakMaxRaw == 210u &&
      c.base.waitMinMs == 900u && c.base.waitMaxMs == 2000u &&
      c.base.refractoryMinMs == 500u && c.base.refractoryMaxMs == 1500u &&
      c.carrierMinRaw == 10u && c.carrierMaxRaw == 60u &&
      !c.visualRescueEnabled && c.visualRescueDelayMs == 500u;
  if (!untouchedV16Default) return false;
  c = nagHumanV4DefaultConfigPure();
  return true;
}

static inline bool nagHumanV4MigrateV17DefaultPure(NagHumanV4ConfigPure &c) {
  const bool exactV17Default =
      c.base.peakMinRaw == 180u && c.base.peakMaxRaw == 260u &&
      c.base.waitMinMs == 900u && c.base.waitMaxMs == 3000u &&
      c.base.refractoryMinMs == 500u && c.base.refractoryMaxMs == 1500u &&
      c.carrierMinRaw == 10u && c.carrierMaxRaw == 60u &&
      c.hoPolicy == H3_HO_TIERED_1_2 &&
      c.ho1ThresholdRaw == 40u && c.ho2ThresholdRaw == 175u &&
      c.visualRescueEnabled && c.visualRescueDelayMs == 500u;
  if (!exactV17Default) return false;
  c = nagHumanV4DefaultConfigPure();
  return true;
}

static inline bool nagHumanV4ConfigValidPure(const NagHumanV4ConfigPure &c) {
  return nagHumanV1PeakRangeValidPure(c.base.peakMinRaw, c.base.peakMaxRaw) &&
         nagHumanV1TimingValidPure(c.base.waitMinMs, c.base.waitMaxMs,
                                   c.base.refractoryMinMs, c.base.refractoryMaxMs) &&
         c.carrierMinRaw >= NAG_HUMAN_V4_CARRIER_MIN_ALLOWED_RAW &&
         c.carrierMaxRaw <= NAG_HUMAN_V4_CARRIER_MAX_ALLOWED_RAW &&
         c.carrierMinRaw <= c.carrierMaxRaw &&
         nagHumanV3HoPolicyValidPure(c.hoPolicy) &&
         c.ho1ThresholdRaw >= NAG_HUMAN_V3_HO_THRESHOLD_MIN_RAW &&
         c.ho2ThresholdRaw <= NAG_HUMAN_V3_HO_THRESHOLD_MAX_RAW &&
         c.ho1ThresholdRaw <= c.ho2ThresholdRaw &&
         c.visualRescueDelayMs <= NAG_HUMAN_V4_VISUAL_RESCUE_MAX_DELAY_MS &&
         c.base.hoOverridePct <= 100u;
}

static inline uint32_t nagHumanV4SanitizeSeedPure(uint32_t seed) {
  return nagHumanV1SanitizeSeedPure(seed ^ 0x52345634u);
}

static inline void nagHumanV4InitPure(NagHumanV4StatePure &s, uint32_t seed) {
  s = NagHumanV4StatePure{};
  nagHumanV1InitPure(s.base, seed);
  s.carrierRngState = nagHumanV4SanitizeSeedPure(seed);
}

static inline void nagHumanV4ResetRuntimePure(NagHumanV4StatePure &s, uint8_t phase) {
  const uint32_t carrierRng = s.carrierRngState;
  const uint32_t visualWarningEpochSeen = s.visualWarningEpochSeen;
  const uint32_t visualRescueCount = s.visualRescueCount;
  nagHumanV1ResetRuntimePure(s.base, phase);
  s.carrierRngState = nagHumanV4SanitizeSeedPure(carrierRng);
  s.lastCarrierMagnitudeRaw = 0u;
  s.lastCarrierApplied = false;
  s.rampOutTargetRaw = NAG_HUMAN_V1_TORQUE_CENTER_RAW;
  s.rampOutTargetValid = false;
  s.visualWarningEpochSeen = visualWarningEpochSeen;
  s.visualRescueCount = visualRescueCount;
  s.visualRescuePendingSinceMs = 0u;
  s.visualRescuePending = false;
}

static inline void nagHumanV4ReseedPure(NagHumanV4StatePure &s, uint32_t seed) {
  const uint32_t events = s.base.eventCount;
  const uint32_t simple = s.base.simpleCount;
  const uint32_t correction = s.base.correctionCount;
  const uint32_t visualWarningEpochSeen = s.visualWarningEpochSeen;
  const uint32_t visualRescueCount = s.visualRescueCount;
  nagHumanV4InitPure(s, seed);
  s.base.eventCount = events;
  s.base.simpleCount = simple;
  s.base.correctionCount = correction;
  s.visualWarningEpochSeen = visualWarningEpochSeen;
  s.visualRescueCount = visualRescueCount;
}

static inline NagHumanV1EventPure nagHumanV4GenerateEventPure(
    NagHumanV4StatePure &s, const NagHumanV4ConfigPure &c) {
  NagHumanV1EventPure e = {};
  e.waitDurationMs = nagHumanV1RangePure(s.base.rngState, c.base.waitMinMs, c.base.waitMaxMs);
  e.rampInDurationMs = nagHumanV1RangePure(s.base.rngState, c.base.rampInMinMs, c.base.rampInMaxMs);
  e.interactDurationMs = nagHumanV1RangePure(s.base.rngState, c.base.interactMinMs, c.base.interactMaxMs);
  e.rampOutDurationMs = nagHumanV1RangePure(s.base.rngState, c.base.rampOutMinMs, c.base.rampOutMaxMs);
  e.refractoryDurationMs = nagHumanV1RangePure(
      s.base.rngState, c.base.refractoryMinMs, c.base.refractoryMaxMs);
  e.peakRaw = (uint16_t)nagHumanV1RangePure(
      s.base.rngState, c.base.peakMinRaw, c.base.peakMaxRaw);
  e.correctionRaw = (uint16_t)nagHumanV1RangePure(
      s.base.rngState, c.base.correctionMinRaw, c.base.correctionMaxRaw);

  const uint32_t directionRoll = nagHumanV1RangePure(s.base.rngState, 0u, 99u);
  e.direction = directionRoll < NAG_HUMAN_V4_NEGATIVE_BIAS_PCT ? -1 : 1;
  s.base.previousDirection = e.direction;

  const uint32_t typeRoll = nagHumanV1RangePure(s.base.rngState, 0u, 99u);
  e.type = typeRoll < c.base.correctionProbabilityPct ? H1_EVENT_CORRECTION : H1_EVENT_SIMPLE;
  return e;
}

static inline void nagHumanV4BeginWaitPure(
    NagHumanV4StatePure &s, const NagHumanV4ConfigPure &c,
    uint32_t nowMs, bool newSession) {
  s.base.event = nagHumanV4GenerateEventPure(s, c);
  s.base.eventValid = true;
  s.base.phase = H1_WAIT;
  s.base.phaseStartMs = nowMs;
  s.base.eventStartMs = 0u;
  if (newSession || s.base.sessionStartMs == 0u) s.base.sessionStartMs = nowMs;
  s.base.carrier = true;
}

static inline void nagHumanV4BeginVisualRescuePeakPure(
    NagHumanV4StatePure &s, const NagHumanV4ConfigPure &c, uint32_t nowMs) {
  s.base.event = nagHumanV4GenerateEventPure(s, c);
  s.base.eventValid = true;
  s.base.phase = H1_INTERACT;
  s.base.phaseStartMs = nowMs;
  s.base.eventStartMs = nowMs;
  if (s.base.sessionStartMs == 0u) s.base.sessionStartMs = nowMs;
  s.base.eventCount++;
  if (s.base.event.type == H1_EVENT_CORRECTION) s.base.correctionCount++;
  else s.base.simpleCount++;
  s.base.carrier = false;
  s.lastCarrierApplied = false;
  s.rampOutTargetValid = false;
  s.visualRescuePending = false;
  s.visualRescuePendingSinceMs = 0u;
  s.visualRescueCount++;
}

static inline bool nagHumanV4UpdateVisualRescuePure(
    NagHumanV4StatePure &s,
    const NagHumanV4ConfigPure &c,
    uint32_t nowMs,
    uint32_t visualWarningEpoch,
    uint32_t visualWarningEnterMs,
    bool visualWarningActive) {
  if (visualWarningEpoch != s.visualWarningEpochSeen) {
    s.visualWarningEpochSeen = visualWarningEpoch;
    s.visualRescuePending = c.visualRescueEnabled &&
                            visualWarningEpoch != 0u &&
                            visualWarningActive;
    s.visualRescuePendingSinceMs = s.visualRescuePending ? visualWarningEnterMs : 0u;
  } else if (!c.visualRescueEnabled || !visualWarningActive) {
    s.visualRescuePending = false;
    s.visualRescuePendingSinceMs = 0u;
  }

  if (!s.visualRescuePending) return false;
  const uint32_t elapsed = (uint32_t)(nowMs - s.visualRescuePendingSinceMs);
  if (elapsed < c.visualRescueDelayMs) return false;
  nagHumanV4BeginVisualRescuePeakPure(s, c, nowMs);
  return true;
}

static inline uint16_t nagHumanV4CarrierOutputPure(
    NagHumanV4StatePure &s, const NagHumanV4ConfigPure &c, uint16_t sourceRaw) {
  s.lastCarrierMagnitudeRaw = (uint16_t)nagHumanV1RangePure(
      s.carrierRngState, c.carrierMinRaw, c.carrierMaxRaw);

  const int32_t stockSigned = (int32_t)sourceRaw - (int32_t)NAG_HUMAN_V1_TORQUE_CENTER_RAW;
  const int32_t stockAbs = stockSigned < 0 ? -stockSigned : stockSigned;
  if (stockAbs <= (int32_t)NAG_HUMAN_V4_STOCK_DEADBAND_RAW) {
    s.lastCarrierApplied = false;
    return sourceRaw;
  }

  const int32_t magnitude = stockAbs + (int32_t)s.lastCarrierMagnitudeRaw;
  const int32_t oppositeSigned = stockSigned > 0 ? -magnitude : magnitude;
  s.lastCarrierApplied = true;
  return nagHumanV1ClampRawPure((int32_t)NAG_HUMAN_V1_TORQUE_CENTER_RAW + oppositeSigned);
}

static inline NagHumanV1StepResultPure nagHumanV4StepPure(
    NagHumanV4StatePure &s,
    const NagHumanV4ConfigPure &c,
    uint32_t nowMs,
    uint16_t sourceRaw,
    bool runAllowed,
    bool speedValid,
    bool speedFresh,
    uint16_t speedRaw,
    uint32_t visualWarningEpoch = 0u,
    uint32_t visualWarningEnterMs = 0u,
    bool visualWarningActive = false) {
  NagHumanV1StepResultPure result = {};
  result.raw = sourceRaw;
  result.phase = s.base.phase;
  result.motion = s.base.motion;

  if (!runAllowed) {
    nagHumanV4ResetRuntimePure(s, H1_IDLE);
    result.phase = s.base.phase;
    result.motion = s.base.motion;
    result.blockReason = H1_BLOCK_RUN_GATE;
    return result;
  }

  uint8_t motionBlock = H1_BLOCK_NONE;
  if (!nagHumanV1MotionAllowsPure(
          s.base, c.base, nowMs, speedValid, speedFresh, speedRaw, motionBlock)) {
    s.lastCarrierApplied = false;
    result.phase = s.base.phase;
    result.motion = s.base.motion;
    result.blockReason = motionBlock;
    return result;
  }

  const bool visualRescueTriggered = nagHumanV4UpdateVisualRescuePure(
      s, c, nowMs, visualWarningEpoch, visualWarningEnterMs, visualWarningActive);

  if (!visualRescueTriggered &&
      (s.base.phase == H1_IDLE || s.base.phase == H1_PAUSED_STOPPED || !s.base.eventValid)) {
    nagHumanV4BeginWaitPure(s, c, nowMs, true);
  }

  // Preserve the established phase scheduler, but generate Rev.4 events and
  // start each ramp from the most recently transmitted carrier value.
  for (uint8_t transitions = 0; transitions < 8u; transitions++) {
    const uint32_t elapsed = (uint32_t)(nowMs - s.base.phaseStartMs);
    uint32_t duration = 0u;
    switch (s.base.phase) {
      case H1_WAIT: duration = s.base.event.waitDurationMs; break;
      case H1_RAMP_IN: duration = s.base.event.rampInDurationMs; break;
      case H1_INTERACT: duration = s.base.event.interactDurationMs; break;
      case H1_RAMP_OUT: duration = s.base.event.rampOutDurationMs; break;
      case H1_REFRACTORY: duration = s.base.event.refractoryDurationMs; break;
      default: duration = 0u; break;
    }
    if (duration == 0u || elapsed < duration) break;

    s.base.phaseStartMs += duration;
    if (s.base.phase == H1_WAIT) {
      s.base.phase = H1_RAMP_IN;
      s.base.eventStartMs = s.base.phaseStartMs;
      s.base.rampBaselineRaw = s.base.outputRaw != 0u ? s.base.outputRaw : sourceRaw;
      s.base.eventCount++;
      if (s.base.event.type == H1_EVENT_CORRECTION) s.base.correctionCount++;
      else s.base.simpleCount++;
    } else if (s.base.phase == H1_RAMP_IN) {
      s.base.phase = H1_INTERACT;
    } else if (s.base.phase == H1_INTERACT) {
      s.base.phase = H1_RAMP_OUT;
      // Refractory uses the Rev.4 opposite-stock carrier. Capture one carrier
      // target at the start of RAMP_OUT so the event decays toward the
      // upcoming carrier regime instead of snapping back to stock and then
      // flipping sign on the next 0x370 frame. Carrier-state frames still
      // re-sample independently on every RX as specified.
      s.rampOutTargetRaw = nagHumanV4CarrierOutputPure(s, c, sourceRaw);
      s.rampOutTargetValid = true;
      s.lastCarrierApplied = false;
    } else if (s.base.phase == H1_RAMP_OUT) {
      s.base.phase = H1_REFRACTORY;
      s.rampOutTargetValid = false;
    } else if (s.base.phase == H1_REFRACTORY) {
      nagHumanV4BeginWaitPure(s, c, s.base.phaseStartMs, false);
    } else {
      break;
    }
  }

  const uint32_t elapsed = (uint32_t)(nowMs - s.base.phaseStartMs);
  uint16_t raw = sourceRaw;
  bool carrier = false;

  if (s.base.phase == H1_WAIT || s.base.phase == H1_REFRACTORY) {
    raw = nagHumanV4CarrierOutputPure(s, c, sourceRaw);
    carrier = true;
  } else if (s.base.phase == H1_RAMP_IN) {
    s.lastCarrierApplied = false;
    const uint32_t p = nagHumanV1SmoothstepQ15Pure(
        nagHumanV1ProgressQ15Pure(elapsed, s.base.event.rampInDurationMs));
    raw = nagHumanV1LerpRawPure(
        s.base.rampBaselineRaw, nagHumanV1PrimaryTargetRawPure(s.base.event), p);
  } else if (s.base.phase == H1_INTERACT) {
    s.lastCarrierApplied = false;
    const uint16_t primary = nagHumanV1PrimaryTargetRawPure(s.base.event);
    if (s.base.event.type != H1_EVENT_CORRECTION) {
      raw = primary;
    } else {
      const uint32_t correctionStart = (s.base.event.interactDurationMs * 65u) / 100u;
      if (elapsed <= correctionStart || correctionStart >= s.base.event.interactDurationMs) {
        raw = primary;
      } else {
        const uint32_t correctionElapsed = elapsed - correctionStart;
        const uint32_t correctionDuration = s.base.event.interactDurationMs - correctionStart;
        const uint32_t p = nagHumanV1SmoothstepQ15Pure(
            nagHumanV1ProgressQ15Pure(correctionElapsed, correctionDuration));
        raw = nagHumanV1LerpRawPure(
            primary, nagHumanV1CorrectionTargetRawPure(s.base.event), p);
      }
    }
  } else if (s.base.phase == H1_RAMP_OUT) {
    s.lastCarrierApplied = false;
    const uint32_t p = nagHumanV1SmoothstepQ15Pure(
        nagHumanV1ProgressQ15Pure(elapsed, s.base.event.rampOutDurationMs));
    const uint16_t target = s.rampOutTargetValid ? s.rampOutTargetRaw : sourceRaw;
    raw = nagHumanV1LerpRawPure(nagHumanV1FinalEventRawPure(s.base.event), target, p);
  }

  s.base.outputRaw = raw;
  s.base.carrier = carrier;
  result.tx = true;
  const NagHumanV3HoDecisionPure ho = nagHumanV3HoDecisionPure(
      c.hoPolicy, nagHumanV3AbsTorqueRawPure(raw),
      c.ho1ThresholdRaw, c.ho2ThresholdRaw);
  result.hoOverrideValid = ho.overrideValid;
  result.hoLevel = ho.level;
  result.setHo = ho.overrideValid && ho.level == 1u;
  result.carrier = carrier;
  result.raw = raw;
  result.phase = s.base.phase;
  result.motion = s.base.motion;
  result.blockReason = H1_BLOCK_NONE;
  return result;
}
