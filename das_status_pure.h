#pragma once

#include <stdint.h>

struct DasStatusPure {
  bool valid;
  uint8_t apState;
  uint8_t visualState;
};

static inline DasStatusPure dasStatusDecodePure(const uint8_t *data,
                                                uint8_t dlc) {
  DasStatusPure out = {false, 0xFFu, 0xFFu};
  if (!data || dlc < 6u) return out;
  out.valid = true;
  out.apState = data[0] & 0x0Fu;
  out.visualState = (data[5] >> 2) & 0x0Fu;
  return out;
}
