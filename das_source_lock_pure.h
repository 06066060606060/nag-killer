#pragma once

#include <stdint.h>

static constexpr uint16_t DAS_SOURCE_399_ID = 0x399u;
static constexpr uint16_t DAS_SOURCE_39B_ID = 0x39Bu;
static constexpr uint8_t DAS_SOURCE_LOCK_FRAMES = 3u;
static constexpr uint32_t DAS_SOURCE_INTERVAL_MIN_MS = 250u;
static constexpr uint32_t DAS_SOURCE_INTERVAL_MAX_MS = 750u;

enum DasSourceDecisionPure : uint8_t {
  DAS_SOURCE_IGNORED = 0,
  DAS_SOURCE_CANDIDATE,
  DAS_SOURCE_LOCKED_NOW,
  DAS_SOURCE_ACCEPTED,
};

struct DasSourceCandidatePure {
  bool initialized;
  uint8_t lastCounter;
  uint8_t streak;
  uint32_t lastMs;
};

struct DasSourceLockPure {
  bool locked;
  uint16_t lockedId;
  uint8_t lockedCounter;
  DasSourceCandidatePure candidate399;
  DasSourceCandidatePure candidate39b;
};

static inline bool dasSourceLockedPure(const DasSourceLockPure &state) {
  return state.locked;
}

static inline uint16_t dasSourceLockedIdPure(
    const DasSourceLockPure &state) {
  return state.locked ? state.lockedId : 0u;
}

static inline DasSourceCandidatePure *dasSourceCandidatePure(
    DasSourceLockPure &state, uint16_t frameId) {
  if (frameId == DAS_SOURCE_399_ID) return &state.candidate399;
  if (frameId == DAS_SOURCE_39B_ID) return &state.candidate39b;
  return nullptr;
}

static inline DasSourceDecisionPure dasSourceObservePure(
    DasSourceLockPure &state,
    uint16_t frameId,
    const uint8_t *data,
    uint8_t dlc,
    uint32_t nowMs,
    bool extendedFrame,
    bool remoteFrame) {
  if (extendedFrame || remoteFrame || !data || dlc != 8u)
    return DAS_SOURCE_IGNORED;
  if (frameId != DAS_SOURCE_399_ID && frameId != DAS_SOURCE_39B_ID)
    return DAS_SOURCE_IGNORED;

  const uint8_t counter = (uint8_t)((data[6] >> 4u) & 0x0Fu);
  if (state.locked) {
    if (frameId != state.lockedId || counter == state.lockedCounter)
      return DAS_SOURCE_IGNORED;
    state.lockedCounter = counter;
    return DAS_SOURCE_ACCEPTED;
  }

  DasSourceCandidatePure *candidate = dasSourceCandidatePure(state, frameId);
  if (!candidate) return DAS_SOURCE_IGNORED;
  if (!candidate->initialized) {
    candidate->initialized = true;
    candidate->streak = 1u;
  } else {
    const uint8_t expected = (uint8_t)((candidate->lastCounter + 1u) & 0x0Fu);
    const uint32_t intervalMs = (uint32_t)(nowMs - candidate->lastMs);
    const bool cadenceValid = intervalMs >= DAS_SOURCE_INTERVAL_MIN_MS &&
        intervalMs <= DAS_SOURCE_INTERVAL_MAX_MS;
    candidate->streak = counter == expected && cadenceValid
        ? (uint8_t)(candidate->streak + 1u) : 1u;
  }
  candidate->lastCounter = counter;
  candidate->lastMs = nowMs;

  if (candidate->streak < DAS_SOURCE_LOCK_FRAMES)
    return DAS_SOURCE_CANDIDATE;
  state.locked = true;
  state.lockedId = frameId;
  state.lockedCounter = counter;
  return DAS_SOURCE_LOCKED_NOW;
}
