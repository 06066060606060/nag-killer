#pragma once

#include <stdint.h>

enum StandardPartyProfilePure : uint8_t {
  PROFILE_UNSET = 0,
  PROFILE_LEGACY_Y = 1,
  PROFILE_LEGACY_3 = 2,
  PROFILE_HIGHLAND_JUNIPER = 3,
};

static inline bool standardPartyProfileValidPure(uint8_t profile) {
  return profile <= PROFILE_HIGHLAND_JUNIPER;
}

static inline bool standardPartyProfileLegacyPure(uint8_t profile) {
  return profile == PROFILE_LEGACY_Y || profile == PROFILE_LEGACY_3;
}

static inline const char *standardPartyProfileLabelPure(uint8_t profile) {
  switch (profile) {
    case PROFILE_LEGACY_Y: return "Legacy Y";
    case PROFILE_LEGACY_3: return "Legacy 3";
    case PROFILE_HIGHLAND_JUNIPER: return "Highland / Juniper";
    default: return "NOT SELECTED";
  }
}
