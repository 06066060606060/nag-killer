#pragma once

#include <Preferences.h>
#include <nvs.h>
#include <nvs_flash.h>

#include "app_state.h"

static Preferences prefs;

static bool cfgSaveSnapshot(const Config &input) {
  Config value = input;
  configSanitizePure(value);
  if (!configValidatePure(value)) return false;
  if (nvs_flash_init() != ESP_OK) return false;
  if (!prefs.begin("nag-sc", false)) return false;
  // One committed NVS blob prevents mixed generations of individual keys.
  const bool ok = prefs.putBytes("snapshot", &value, sizeof(value)) ==
      sizeof(value);
  prefs.end();
  return ok;
}

static bool cfgReadOptionalU8(nvs_handle_t handle, const char *key,
                              uint8_t &value) {
  const esp_err_t result = nvs_get_u8(handle, key, &value);
  return result == ESP_OK || result == ESP_ERR_NVS_NOT_FOUND;
}

static bool cfgReadOptionalU16(nvs_handle_t handle, const char *key,
                               uint16_t &value) {
  const esp_err_t result = nvs_get_u16(handle, key, &value);
  return result == ESP_OK || result == ESP_ERR_NVS_NOT_FOUND;
}

static bool cfgReadOptionalBool(nvs_handle_t handle, const char *key,
                                bool &value) {
  uint8_t raw = value ? 1u : 0u;
  if (!cfgReadOptionalU8(handle, key, raw)) return false;
  value = raw != 0u;
  return true;
}

static bool cfgReadOptionalBlob(nvs_handle_t handle, const char *key,
                                void *value, size_t expectedSize) {
  size_t size = 0u;
  const esp_err_t result = nvs_get_blob(handle, key, nullptr, &size);
  if (result == ESP_ERR_NVS_NOT_FOUND) return true;
  if (result != ESP_OK || size != expectedSize) return false;
  return nvs_get_blob(handle, key, value, &size) == ESP_OK &&
      size == expectedSize;
}

static bool cfgReadLegacy(nvs_handle_t handle, Config &loaded) {
  uint8_t schema = 0u;
  const esp_err_t result = nvs_get_u8(handle, "schema", &schema);
  if (result == ESP_ERR_NVS_NOT_FOUND) return true;
  if (result != ESP_OK || schema != SC_CONFIG_SCHEMA) return false;
  bool ok = true;
  ok = cfgReadOptionalBool(handle, "enabled", loaded.enabled) && ok;
  ok = cfgReadOptionalU8(handle, "mode", loaded.mode) && ok;
  ok = cfgReadOptionalU8(handle, "profile", loaded.profile) && ok;
  ok = cfgReadOptionalBool(handle, "apOnly", loaded.apOnlyInjection) && ok;
  ok = cfgReadOptionalU8(handle, "tc", loaded.torqueCount) && ok;
  ok = cfgReadOptionalBlob(handle, "tb2", loaded.torqueB2,
                           sizeof(loaded.torqueB2)) && ok;
  ok = cfgReadOptionalBlob(handle, "tb3", loaded.torqueB3,
                           sizeof(loaded.torqueB3)) && ok;
  ok = cfgReadOptionalU8(handle, "ho", loaded.hoRatePct) && ok;
  ok = cfgReadOptionalU16(handle, "burst", loaded.burstMs) && ok;
  ok = cfgReadOptionalU16(handle, "pause", loaded.pauseMs) && ok;
  ok = cfgReadOptionalBlob(handle, "modeH", &loaded.modeH,
                           sizeof(loaded.modeH)) && ok;
  configSanitizePure(loaded);
  return ok;
}

static bool cfgReadPersistent(Config &loaded) {
  if (nvs_flash_init() != ESP_OK) return false;
  nvs_handle_t handle;
  const esp_err_t opened = nvs_open("nag-sc", NVS_READONLY, &handle);
  if (opened == ESP_ERR_NVS_NOT_FOUND) return true;
  if (opened != ESP_OK) return false;
  size_t size = 0u;
  const esp_err_t result = nvs_get_blob(handle, "snapshot", nullptr, &size);
  bool ok = false;
  if (result == ESP_ERR_NVS_NOT_FOUND) {
    // v3.8.2-SC keys remain readable; migration happens on an explicit save.
    ok = cfgReadLegacy(handle, loaded);
  } else if (result == ESP_OK && size == sizeof(loaded)) {
    ok = nvs_get_blob(handle, "snapshot", &loaded, &size) == ESP_OK &&
        size == sizeof(loaded) && configValidatePure(loaded);
  }
  // A present but invalid/unreadable snapshot never falls back to old keys.
  nvs_close(handle);
  return ok;
}

static bool cfgLoad() {
  Config loaded = configDefaultsPure();
  const bool ok = cfgReadPersistent(loaded);
  if (!ok) loaded = configDefaultsPure();
  portENTER_CRITICAL(&cfgMux);
  cfg = loaded;
  portEXIT_CRITICAL(&cfgMux);
  runtimeInvalidate(RUNTIME_RESET_BOOT);
  // Never overwrite an unreadable record or erase NVS during boot.
  return ok;
}

static bool cfgCommit(const Config &input, NagRuntimeResetReason reason) {
  Config value = input;
  configSanitizePure(value);
  if (!cfgSaveSnapshot(value)) return false;
  const bool locked = canTxMutex &&
      xSemaphoreTake(canTxMutex, portMAX_DELAY) == pdTRUE;
  portENTER_CRITICAL(&cfgMux);
  cfg = value;
  portEXIT_CRITICAL(&cfgMux);
  runtimeInvalidate(reason);
  if (locked) xSemaphoreGive(canTxMutex);
  return true;
}

static bool cfgClearPersistent() {
  if (!prefs.begin("nag-sc", false)) return false;
  const bool ok = prefs.clear();
  prefs.end();
  if (!ok) return false;
  const bool locked = canTxMutex &&
      xSemaphoreTake(canTxMutex, portMAX_DELAY) == pdTRUE;
  portENTER_CRITICAL(&cfgMux);
  cfg = configDefaultsPure();
  portEXIT_CRITICAL(&cfgMux);
  runtimeInvalidate(RUNTIME_RESET_FACTORY);
  if (locked) xSemaphoreGive(canTxMutex);
  return true;
}
