#pragma once
#include <stdint.h>
#include "nag_human_v1_pure.h"

// Mode H Rev.3: Rev.1 Plus Human Interaction waveform with a bounded
// stock-relative Natural Grip carrier during WAIT plus selectable HO policy.
// Torque magnitudes use 0.01 Nm raw units around the EPAS center (2050 = 0 Nm).

enum NagHumanV3CarrierDirectionPure : uint8_t {
  H3_CARRIER_FOLLOW_STOCK = 0,
  H3_CARRIER_RANDOM_SIGN = 1
};

enum NagHumanV3HoPolicyPure : uint8_t {
  H3_HO_ALWAYS_1 = 0,
  H3_HO_THRESHOLD_1 = 1,
  H3_HO_TIERED_1_2 = 2
};

static constexpr uint16_t NAG_HUMAN_V3_CARRIER_MIN_ALLOWED_RAW = 10u;   // 0.10 Nm
static constexpr uint16_t NAG_HUMAN_V3_CARRIER_MAX_ALLOWED_RAW = 80u;   // 0.80 Nm
static constexpr uint16_t NAG_HUMAN_V3_HO_THRESHOLD_MIN_RAW = 10u;      // 0.10 Nm
static constexpr uint16_t NAG_HUMAN_V3_HO_THRESHOLD_MAX_RAW = 300u;     // 3.00 Nm

struct NagHumanV3ConfigPure {
  NagHumanV1ConfigPure base;
  uint16_t carrierMinRaw;
  uint16_t carrierMaxRaw;
  uint16_t carrierDwellMinMs;
  uint16_t carrierDwellMaxMs;
  uint8_t carrierDirectionMode;
  uint8_t hoPolicy;
  uint16_t ho1ThresholdRaw;
  uint16_t ho2ThresholdRaw;
};

struct NagHumanV3StatePure {
  NagHumanV1StatePure base;
  uint32_t carrierRngState;
  bool carrierDecisionValid;
  uint32_t carrierDecisionStartMs;
  uint16_t carrierDecisionDurationMs;
  uint16_t carrierMagnitudeRaw;
  int8_t carrierRandomDirection;
  int8_t lastStockDirection;
};

struct NagHumanV3HoDecisionPure {
  bool overrideValid;
  uint8_t level;
};

static inline bool nagHumanV3CarrierDirectionValidPure(uint8_t mode) {
  return mode == H3_CARRIER_FOLLOW_STOCK || mode == H3_CARRIER_RANDOM_SIGN;
}

static inline bool nagHumanV3HoPolicyValidPure(uint8_t mode) {
  return mode == H3_HO_ALWAYS_1 || mode == H3_HO_THRESHOLD_1 || mode == H3_HO_TIERED_1_2;
}

static inline const char* nagHumanV3CarrierDirectionNamePure(uint8_t mode) {
  return mode == H3_CARRIER_RANDOM_SIGN ? "RANDOM_SIGN" : "FOLLOW_STOCK";
}

static inline const char* nagHumanV3HoPolicyNamePure(uint8_t mode) {
  switch (mode) {
    case H3_HO_THRESHOLD_1: return "THRESHOLD_HO1";
    case H3_HO_TIERED_1_2: return "TIERED_HO1_HO2";
    default: return "ALWAYS_HO1";
  }
}

static inline uint8_t nagHumanV3SafeHoLevelPure(uint8_t level) {
  return level > 2u ? 2u : level;
}

static inline NagHumanV3ConfigPure nagHumanV3DefaultConfigPure() {
  NagHumanV3ConfigPure c = {};
  c.base = nagHumanV1Rev1PlusConfigPure();
  // d4 Rev.3 defaults mirror the approved LAB profile. Keep the Rev.1 Plus
  // event engine, but use the tuned production envelope shown in the UI.
  c.base.peakMinRaw = 150u;        // 1.50 Nm
  c.base.peakMaxRaw = 210u;        // 2.10 Nm
  c.base.waitMinMs = 1200u;        // 1.2 s
  c.base.waitMaxMs = 2500u;        // 2.5 s
  c.base.refractoryMinMs = 800u;   // 0.8 s
  c.base.refractoryMaxMs = 1800u;  // 1.8 s
  // Rev.3 owns the HO policy. Disable the legacy percentage engine in the base
  // waveform so it cannot consume or contradict the Rev.3 HO decision.
  c.base.hoOverridePct = 0u;
  c.carrierMinRaw = 30u;          // +0.30 Nm to stock
  c.carrierMaxRaw = 40u;          // +0.40 Nm to stock
  c.carrierDwellMinMs = 350u;
  c.carrierDwellMaxMs = 900u;
  c.carrierDirectionMode = H3_CARRIER_FOLLOW_STOCK;
  c.hoPolicy = H3_HO_TIERED_1_2;
  c.ho1ThresholdRaw = 40u;        // 0.40 Nm
  c.ho2ThresholdRaw = 175u;       // 1.75 Nm
  return c;
}

static inline bool nagHumanV3MigrateD3DefaultToD4Pure(NagHumanV3ConfigPure &c) {
  // Preserve custom Rev.3 profiles. Only migrate either the untouched d3
  // defaults or the approved pre-d4 LAB profile shown in the handoff
  // screenshots (same d4 targets except the old 0.50 Nm HO1 threshold).
  const bool common =
      c.base.peakMinRaw == 150u &&
      c.base.waitMinMs == 1200u &&
      c.base.refractoryMinMs == 800u &&
      c.base.refractoryMaxMs == 1800u &&
      c.carrierMinRaw == 30u && c.carrierMaxRaw == 40u &&
      c.carrierDirectionMode == H3_CARRIER_FOLLOW_STOCK &&
      c.hoPolicy == H3_HO_TIERED_1_2;
  const bool untouchedD3 = common &&
      c.base.peakMaxRaw == 220u && c.base.waitMaxMs == 3000u &&
      c.ho1ThresholdRaw == 50u && c.ho2ThresholdRaw == 160u;
  const bool approvedPreD4 = common &&
      c.base.peakMaxRaw == 210u && c.base.waitMaxMs == 2500u &&
      c.ho1ThresholdRaw == 50u && c.ho2ThresholdRaw == 175u;
  if (!untouchedD3 && !approvedPreD4) return false;
  c = nagHumanV3DefaultConfigPure();
  return true;
}

static inline bool nagHumanV3ConfigValidPure(const NagHumanV3ConfigPure &c) {
  return nagHumanV1PeakRangeValidPure(c.base.peakMinRaw, c.base.peakMaxRaw) &&
         nagHumanV1TimingValidPure(c.base.waitMinMs, c.base.waitMaxMs,
                                   c.base.refractoryMinMs, c.base.refractoryMaxMs) &&
         c.carrierMinRaw >= NAG_HUMAN_V3_CARRIER_MIN_ALLOWED_RAW &&
         c.carrierMaxRaw <= NAG_HUMAN_V3_CARRIER_MAX_ALLOWED_RAW &&
         c.carrierMinRaw <= c.carrierMaxRaw &&
         c.carrierDwellMinMs >= 100u && c.carrierDwellMaxMs <= 3000u &&
         c.carrierDwellMinMs <= c.carrierDwellMaxMs &&
         nagHumanV3CarrierDirectionValidPure(c.carrierDirectionMode) &&
         nagHumanV3HoPolicyValidPure(c.hoPolicy) &&
         c.ho1ThresholdRaw >= NAG_HUMAN_V3_HO_THRESHOLD_MIN_RAW &&
         c.ho2ThresholdRaw <= NAG_HUMAN_V3_HO_THRESHOLD_MAX_RAW &&
         c.ho1ThresholdRaw <= c.ho2ThresholdRaw;
}

static inline uint32_t nagHumanV3SanitizeSeedPure(uint32_t seed) {
  return nagHumanV1SanitizeSeedPure(seed ^ 0x52335633u);
}

static inline void nagHumanV3InitPure(NagHumanV3StatePure &s, uint32_t seed) {
  s = NagHumanV3StatePure{};
  nagHumanV1InitPure(s.base, seed);
  s.carrierRngState = nagHumanV3SanitizeSeedPure(seed);
}

static inline void nagHumanV3ResetRuntimePure(NagHumanV3StatePure &s, uint8_t phase) {
  const uint32_t carrierRng = s.carrierRngState;
  nagHumanV1ResetRuntimePure(s.base, phase);
  s.carrierDecisionValid = false;
  s.carrierDecisionStartMs = 0u;
  s.carrierDecisionDurationMs = 0u;
  s.carrierMagnitudeRaw = 0u;
  s.carrierRandomDirection = 0;
  s.lastStockDirection = 0;
  s.carrierRngState = nagHumanV3SanitizeSeedPure(carrierRng);
}

static inline void nagHumanV3ReseedPure(NagHumanV3StatePure &s, uint32_t seed) {
  const uint32_t events = s.base.eventCount;
  const uint32_t simple = s.base.simpleCount;
  const uint32_t correction = s.base.correctionCount;
  nagHumanV3InitPure(s, seed);
  s.base.eventCount = events;
  s.base.simpleCount = simple;
  s.base.correctionCount = correction;
}

static inline uint16_t nagHumanV3AbsTorqueRawPure(uint16_t raw) {
  return raw >= NAG_HUMAN_V1_TORQUE_CENTER_RAW
      ? (uint16_t)(raw - NAG_HUMAN_V1_TORQUE_CENTER_RAW)
      : (uint16_t)(NAG_HUMAN_V1_TORQUE_CENTER_RAW - raw);
}

static inline NagHumanV3HoDecisionPure nagHumanV3HoDecisionPure(
    uint8_t policy, uint16_t torqueMagnitudeRaw,
    uint16_t ho1ThresholdRaw, uint16_t ho2ThresholdRaw) {
  NagHumanV3HoDecisionPure r = {false, 0u};
  if (!nagHumanV3HoPolicyValidPure(policy)) return r;
  if (policy == H3_HO_ALWAYS_1) {
    r.overrideValid = true;
    r.level = 1u;
  } else if (policy == H3_HO_THRESHOLD_1) {
    if (torqueMagnitudeRaw >= ho1ThresholdRaw) {
      r.overrideValid = true;
      r.level = 1u;
    }
  } else {
    if (torqueMagnitudeRaw >= ho2ThresholdRaw) {
      r.overrideValid = true;
      r.level = 2u;
    } else if (torqueMagnitudeRaw >= ho1ThresholdRaw) {
      r.overrideValid = true;
      r.level = 1u;
    }
  }
  r.level = nagHumanV3SafeHoLevelPure(r.level);
  return r;
}

static inline void nagHumanV3RefreshCarrierDecisionPure(
    NagHumanV3StatePure &s, const NagHumanV3ConfigPure &c, uint32_t nowMs) {
  const bool expired = !s.carrierDecisionValid ||
      (uint32_t)(nowMs - s.carrierDecisionStartMs) >= s.carrierDecisionDurationMs;
  if (!expired) return;
  s.carrierMagnitudeRaw = (uint16_t)nagHumanV1RangePure(
      s.carrierRngState, c.carrierMinRaw, c.carrierMaxRaw);
  s.carrierDecisionDurationMs = (uint16_t)nagHumanV1RangePure(
      s.carrierRngState, c.carrierDwellMinMs, c.carrierDwellMaxMs);
  s.carrierDecisionStartMs = nowMs;
  s.carrierDecisionValid = true;
  if (c.carrierDirectionMode == H3_CARRIER_RANDOM_SIGN) {
    s.carrierRandomDirection = (nagHumanV1NextRandomPure(s.carrierRngState) & 1u) ? 1 : -1;
  }
}

static inline int8_t nagHumanV3CarrierDirectionPure(
    NagHumanV3StatePure &s, const NagHumanV3ConfigPure &c, uint16_t sourceRaw) {
  if (c.carrierDirectionMode == H3_CARRIER_RANDOM_SIGN) {
    if (s.carrierRandomDirection != 1 && s.carrierRandomDirection != -1)
      s.carrierRandomDirection = 1;
    return s.carrierRandomDirection;
  }
  if (sourceRaw > NAG_HUMAN_V1_TORQUE_CENTER_RAW) s.lastStockDirection = 1;
  else if (sourceRaw < NAG_HUMAN_V1_TORQUE_CENTER_RAW) s.lastStockDirection = -1;
  if (s.lastStockDirection != 1 && s.lastStockDirection != -1)
    s.lastStockDirection = 1;
  return s.lastStockDirection;
}

static inline NagHumanV1StepResultPure nagHumanV3StepPure(
    NagHumanV3StatePure &s,
    const NagHumanV3ConfigPure &c,
    uint32_t nowMs,
    uint16_t sourceRaw,
    bool runAllowed,
    bool speedValid,
    bool speedFresh,
    uint16_t speedRaw) {
  const uint8_t previousPhase = s.base.phase;
  const uint16_t previousOutputRaw = s.base.outputRaw;
  NagHumanV1StepResultPure r = nagHumanV1StepPure(
      s.base, c.base, nowMs, sourceRaw, runAllowed, speedValid, speedFresh, speedRaw);
  if (!r.tx) return r;

  // If WAIT just transitioned into RAMP_IN, start the event from the most recent
  // Rev.3 carrier output rather than snapping back to the stock baseline first.
  if (previousPhase == H1_WAIT && r.phase == H1_RAMP_IN &&
      previousOutputRaw != 0u) {
    s.base.rampBaselineRaw = previousOutputRaw;
    const uint32_t elapsed = (uint32_t)(nowMs - s.base.phaseStartMs);
    const uint32_t p = nagHumanV1SmoothstepQ15Pure(
        nagHumanV1ProgressQ15Pure(elapsed, s.base.event.rampInDurationMs));
    r.raw = nagHumanV1LerpRawPure(s.base.rampBaselineRaw,
                                  nagHumanV1PrimaryTargetRawPure(s.base.event), p);
  }

  if (r.phase == H1_WAIT) {
    nagHumanV3RefreshCarrierDecisionPure(s, c, nowMs);
    const int8_t direction = nagHumanV3CarrierDirectionPure(s, c, sourceRaw);
    r.raw = nagHumanV1ClampRawPure((int32_t)sourceRaw +
        (int32_t)direction * (int32_t)s.carrierMagnitudeRaw);
    r.carrier = true;
  }

  s.base.outputRaw = r.raw;
  s.base.carrier = r.carrier;

  const NagHumanV3HoDecisionPure ho = nagHumanV3HoDecisionPure(
      c.hoPolicy, nagHumanV3AbsTorqueRawPure(r.raw),
      c.ho1ThresholdRaw, c.ho2ThresholdRaw);
  r.hoOverrideValid = ho.overrideValid;
  r.hoLevel = ho.level;
  r.setHo = ho.overrideValid && ho.level == 1u;
  return r;
}
