#pragma once

#include <stdint.h>

enum NagBlockReasonPure : uint8_t {
  NAG_BLOCK_NONE = 0,
  NAG_BLOCK_PROFILE_NOT_SET,
  NAG_BLOCK_NAG_OFF,
  NAG_BLOCK_RESTORING,
  NAG_BLOCK_TWAI_NOT_READY,
  NAG_BLOCK_OTA_ACTIVE,
  NAG_BLOCK_EPAS_STALE,
  NAG_BLOCK_SELF_ECHO,
  NAG_BLOCK_STOCK_HANDS_ON,
  NAG_BLOCK_AP_INACTIVE,
  NAG_BLOCK_DAS_STALE,
  NAG_BLOCK_SPEED_STALE,
  NAG_BLOCK_TORQUE_LIMIT,
};

struct NagSafetyInputPure {
  bool profileSelected;
  bool requestedOn;
  bool effectiveOn;
  bool twaiReady;
  bool otaActive;
  bool epasFresh;
  bool selfEcho;
  bool stockHandsOnSupported;
  bool torqueWithinLimit;
  bool apOnlyInjection;
  bool dasRequired;
  bool dasValid;
  bool dasFresh;
  bool apActive;
  bool modeH;
  bool speedValid;
  bool speedFresh;
};

struct NagSafetyDecisionPure {
  bool allowed;
  uint8_t reason;
};

static inline const char *nagBlockReasonLabelPure(uint8_t reason) {
  switch (reason) {
    case NAG_BLOCK_NONE: return "READY";
    case NAG_BLOCK_PROFILE_NOT_SET: return "PROFILE NOT SET";
    case NAG_BLOCK_NAG_OFF: return "NAG OFF";
    case NAG_BLOCK_RESTORING: return "RESTORING";
    case NAG_BLOCK_TWAI_NOT_READY: return "CAN NOT READY";
    case NAG_BLOCK_OTA_ACTIVE: return "OTA ACTIVE";
    case NAG_BLOCK_EPAS_STALE: return "EPAS STALE";
    case NAG_BLOCK_SELF_ECHO: return "SELF ECHO";
    case NAG_BLOCK_STOCK_HANDS_ON: return "STOCK HANDS ON";
    case NAG_BLOCK_AP_INACTIVE: return "AP INACTIVE";
    case NAG_BLOCK_DAS_STALE: return "DAS STALE";
    case NAG_BLOCK_SPEED_STALE: return "SPEED STALE";
    case NAG_BLOCK_TORQUE_LIMIT: return "TORQUE LIMIT";
    default: return "UNKNOWN";
  }
}

static inline NagSafetyDecisionPure nagEvaluateSafetyPure(
    const NagSafetyInputPure &input) {
  uint8_t reason = NAG_BLOCK_NONE;
  if (!input.profileSelected) reason = NAG_BLOCK_PROFILE_NOT_SET;
  else if (!input.requestedOn) reason = NAG_BLOCK_NAG_OFF;
  else if (!input.effectiveOn) reason = NAG_BLOCK_RESTORING;
  else if (!input.twaiReady) reason = NAG_BLOCK_TWAI_NOT_READY;
  else if (input.otaActive) reason = NAG_BLOCK_OTA_ACTIVE;
  else if (!input.epasFresh) reason = NAG_BLOCK_EPAS_STALE;
  else if (input.selfEcho) reason = NAG_BLOCK_SELF_ECHO;
  else if (!input.stockHandsOnSupported) reason = NAG_BLOCK_STOCK_HANDS_ON;
  else if (!input.torqueWithinLimit) reason = NAG_BLOCK_TORQUE_LIMIT;
  else if (input.dasRequired && (!input.dasValid || !input.dasFresh))
    reason = NAG_BLOCK_DAS_STALE;
  else if (input.apOnlyInjection && !input.apActive) reason = NAG_BLOCK_AP_INACTIVE;
  else if (input.modeH && (!input.speedValid || !input.speedFresh))
    reason = NAG_BLOCK_SPEED_STALE;
  return {reason == NAG_BLOCK_NONE, reason};
}
