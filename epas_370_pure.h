#pragma once

#include <stdint.h>

#include "config_model_pure.h"

struct Epas370SourcePure {
  uint16_t torqueRaw;
  uint8_t handsOn;
};

static inline bool epas370DecodeSourcePure(const uint8_t *data, uint8_t dlc,
                                           Epas370SourcePure &out) {
  if (!data || dlc != 8u) return false;
  out.torqueRaw = (uint16_t)(((uint16_t)(data[2] & 0x0Fu) << 8) | data[3]);
  out.handsOn = (uint8_t)((data[4] >> 6) & 0x03u);
  return true;
}

static inline uint16_t epas370ClampTorqueRawPure(uint16_t raw) {
  if (raw < SC_TORQUE_RAW_MIN) return SC_TORQUE_RAW_MIN;
  if (raw > SC_TORQUE_RAW_MAX) return SC_TORQUE_RAW_MAX;
  return raw;
}

static inline bool epas370ComposePure(const uint8_t *source, uint8_t dlc,
                                     uint16_t torqueRaw,
                                     bool hoOverrideValid, uint8_t hoLevel,
                                     uint8_t *out) {
  if (!source || !out || dlc != 8u) return false;
  if (hoOverrideValid && hoLevel > 2u) return false;
  for (uint8_t i = 0u; i < 8u; ++i) out[i] = source[i];
  torqueRaw = epas370ClampTorqueRawPure(torqueRaw);
  out[2] = (uint8_t)((source[2] & 0xF0u) | ((torqueRaw >> 8) & 0x0Fu));
  out[3] = (uint8_t)(torqueRaw & 0xFFu);
  if (hoOverrideValid)
    out[4] = (uint8_t)((source[4] & 0x3Fu) | ((hoLevel & 0x03u) << 6));
  out[6] = (uint8_t)((source[6] & 0xF0u) |
                     (((source[6] & 0x0Fu) + 1u) & 0x0Fu));
  uint16_t sum = 0u;
  for (uint8_t i = 0u; i < 7u; ++i) sum += out[i];
  out[7] = (uint8_t)((sum + 0x73u) & 0xFFu);
  return true;
}

static inline bool epas370ExactRecentEchoPure(
    bool modeH, bool lastTxValid, uint32_t ageMs,
    const uint8_t *received, const uint8_t *lastTx, uint8_t dlc) {
  if (!modeH || !lastTxValid || ageMs > 20u || !received || !lastTx || dlc != 8u)
    return false;
  for (uint8_t i = 0u; i < 8u; ++i) {
    if (received[i] != lastTx[i]) return false;
  }
  return true;
}
