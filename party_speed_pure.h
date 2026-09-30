#pragma once

#include <stdint.h>

struct PartySpeedPure {
  bool valid;
  uint16_t raw;
  int32_t kphX100;
};

static inline PartySpeedPure partySpeedDecodePure(const uint8_t *data,
                                                  uint8_t dlc) {
  PartySpeedPure out = {false, 0u, 0};
  if (!data || dlc < 3u) return out;
  const uint16_t raw =
      (uint16_t)(((uint16_t)(data[1] >> 4) & 0x0Fu) |
                 ((uint16_t)data[2] << 4));
  if (raw == 0x0FFFu) return out;
  out.valid = true;
  out.raw = raw;
  out.kphX100 = (int32_t)raw * 8 - 4000;
  return out;
}
