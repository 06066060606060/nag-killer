#pragma once

#include <stdint.h>

#include "mode_h_rev4_sc_pure.h"
#include "standard_party_profile_pure.h"

static constexpr uint8_t SC_CONFIG_SCHEMA = 1u;
static constexpr uint8_t SC_MAX_TORQUE_ENTRIES = 8u;
static constexpr uint16_t SC_TORQUE_RAW_MIN = 1870u;
static constexpr uint16_t SC_TORQUE_RAW_MAX = 2230u;

enum NagMode : uint8_t {
  MODE_A = 0,
  MODE_B = 1,
  MODE_C = 2,
  MODE_H = 3,
};

struct Config {
  uint8_t schema;
  bool enabled;
  uint8_t mode;
  uint8_t profile;
  bool apOnlyInjection;
  uint8_t torqueCount;
  uint8_t torqueB2[SC_MAX_TORQUE_ENTRIES];
  uint8_t torqueB3[SC_MAX_TORQUE_ENTRIES];
  uint8_t hoRatePct;
  uint16_t burstMs;
  uint16_t pauseMs;
  NagHumanV4ConfigPure modeH;
};

static inline uint16_t configTorqueRawPure(const Config &config,
                                           uint8_t index) {
  if (index >= SC_MAX_TORQUE_ENTRIES) return NAG_HUMAN_V1_TORQUE_CENTER_RAW;
  return (uint16_t)(((uint16_t)(config.torqueB2[index] & 0x0Fu) << 8) |
                    config.torqueB3[index]);
}

static inline void configSetTorqueRawPure(Config &config, uint8_t index,
                                          uint16_t raw) {
  if (index >= SC_MAX_TORQUE_ENTRIES) return;
  if (raw < SC_TORQUE_RAW_MIN) raw = SC_TORQUE_RAW_MIN;
  if (raw > SC_TORQUE_RAW_MAX) raw = SC_TORQUE_RAW_MAX;
  config.torqueB2[index] = (uint8_t)((raw >> 8) & 0x0Fu);
  config.torqueB3[index] = (uint8_t)(raw & 0xFFu);
}

static inline Config configDefaultsPure() {
  Config config = {};
  config.schema = SC_CONFIG_SCHEMA;
  config.enabled = false;
  config.mode = MODE_A;
  config.profile = PROFILE_UNSET;
  config.apOnlyInjection = true;
  config.torqueCount = 1u;
  configSetTorqueRawPure(config, 0u, SC_TORQUE_RAW_MAX);
  config.hoRatePct = 100u;
  config.burstMs = 1000u;
  config.pauseMs = 1500u;
  config.modeH = nagHumanV4DefaultConfigPure();
  return config;
}

static inline bool configApOnlyEditablePure(const Config &config) {
  return !standardPartyProfileLegacyPure(config.profile);
}

static inline bool configDasRequiredPure(const Config &config) {
  return !standardPartyProfileLegacyPure(config.profile);
}

static inline void configApplyProfilePure(Config &config, uint8_t profile) {
  config.profile = standardPartyProfileValidPure(profile)
      ? profile : PROFILE_UNSET;
  if (config.profile == PROFILE_UNSET) config.enabled = false;
  if (!configApOnlyEditablePure(config)) config.apOnlyInjection = false;
}

static inline void configApplyModeDefaultsPure(Config &config, uint8_t mode) {
  const bool enabled = config.enabled;
  const uint8_t profile = config.profile;
  const bool apOnly = config.apOnlyInjection;
  config = configDefaultsPure();
  config.enabled = enabled;
  config.profile = profile;
  config.apOnlyInjection = apOnly;
  config.mode = mode <= MODE_H ? mode : MODE_A;
  if (config.mode == MODE_B) {
    config.torqueCount = 4u;
    configSetTorqueRawPure(config, 0u, 2230u);
    configSetTorqueRawPure(config, 1u, 2200u);
    configSetTorqueRawPure(config, 2u, 1900u);
    configSetTorqueRawPure(config, 3u, 1870u);
  }
}

static inline bool configValidatePure(const Config &config) {
  if (config.schema != SC_CONFIG_SCHEMA) return false;
  if (config.mode > MODE_H) return false;
  if (!standardPartyProfileValidPure(config.profile)) return false;
  if (config.enabled && config.profile == PROFILE_UNSET) return false;
  if (!configApOnlyEditablePure(config) && config.apOnlyInjection) return false;
  if (config.torqueCount < 1u || config.torqueCount > SC_MAX_TORQUE_ENTRIES)
    return false;
  if (config.hoRatePct > 100u) return false;
  if (config.burstMs < 50u || config.burstMs > 10000u) return false;
  if (config.pauseMs > 10000u) return false;
  for (uint8_t i = 0u; i < config.torqueCount; ++i) {
    const uint16_t raw = configTorqueRawPure(config, i);
    if (raw < SC_TORQUE_RAW_MIN || raw > SC_TORQUE_RAW_MAX) return false;
  }
  return nagHumanV4ConfigValidPure(config.modeH);
}

static inline void configSanitizePure(Config &config) {
  if (config.schema != SC_CONFIG_SCHEMA) {
    config = configDefaultsPure();
    return;
  }
  if (config.mode > MODE_H) config.mode = MODE_A;
  if (!standardPartyProfileValidPure(config.profile))
    config.profile = PROFILE_UNSET;
  if (config.profile == PROFILE_UNSET) config.enabled = false;
  if (!configApOnlyEditablePure(config)) config.apOnlyInjection = false;
  if (config.torqueCount < 1u) config.torqueCount = 1u;
  if (config.torqueCount > SC_MAX_TORQUE_ENTRIES)
    config.torqueCount = SC_MAX_TORQUE_ENTRIES;
  if (config.hoRatePct > 100u) config.hoRatePct = 100u;
  if (config.burstMs < 50u) config.burstMs = 50u;
  if (config.burstMs > 10000u) config.burstMs = 10000u;
  if (config.pauseMs > 10000u) config.pauseMs = 10000u;
  for (uint8_t i = 0u; i < config.torqueCount; ++i)
    configSetTorqueRawPure(config, i, configTorqueRawPure(config, i));
  if (!nagHumanV4ConfigValidPure(config.modeH))
    config.modeH = nagHumanV4DefaultConfigPure();
}
