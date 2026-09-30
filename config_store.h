#pragma once

#include <Preferences.h>
#include <nvs_flash.h>

#include "app_state.h"

static Preferences prefs;

static bool cfgSaveSnapshot(const Config &input) {
  Config value = input;
  configSanitizePure(value);
  if (!prefs.begin("nag-sc", false)) return false;
  bool ok = true;
  ok = prefs.putUChar("schema", value.schema) > 0u && ok;
  ok = prefs.putBool("enabled", value.enabled) > 0u && ok;
  ok = prefs.putUChar("mode", value.mode) > 0u && ok;
  ok = prefs.putUChar("profile", value.profile) > 0u && ok;
  ok = prefs.putBool("apOnly", value.apOnlyInjection) > 0u && ok;
  ok = prefs.putUChar("tc", value.torqueCount) > 0u && ok;
  ok = prefs.putBytes("tb2", value.torqueB2, sizeof(value.torqueB2)) ==
           sizeof(value.torqueB2) && ok;
  ok = prefs.putBytes("tb3", value.torqueB3, sizeof(value.torqueB3)) ==
           sizeof(value.torqueB3) && ok;
  ok = prefs.putUChar("ho", value.hoRatePct) > 0u && ok;
  ok = prefs.putUShort("burst", value.burstMs) > 0u && ok;
  ok = prefs.putUShort("pause", value.pauseMs) > 0u && ok;
  ok = prefs.putBytes("modeH", &value.modeH, sizeof(value.modeH)) ==
           sizeof(value.modeH) && ok;
  prefs.end();
  return ok;
}

static void cfgLoad() {
  Config loaded = configDefaultsPure();
  esp_err_t nvsResult = nvs_flash_init();
  if (nvsResult == ESP_ERR_NVS_NO_FREE_PAGES ||
      nvsResult == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    if (nvs_flash_erase() == ESP_OK) nvsResult = nvs_flash_init();
  }

  bool found = false;
  if (nvsResult == ESP_OK && prefs.begin("nag-sc", true)) {
    const uint8_t schema = prefs.getUChar("schema", 0u);
    if (schema == SC_CONFIG_SCHEMA) {
      found = true;
      loaded.schema = schema;
      loaded.enabled = prefs.getBool("enabled", false);
      loaded.mode = prefs.getUChar("mode", MODE_A);
      loaded.profile = prefs.getUChar("profile", PROFILE_UNSET);
      loaded.apOnlyInjection = prefs.getBool("apOnly", true);
      loaded.torqueCount = prefs.getUChar("tc", 1u);
      (void)prefs.getBytes("tb2", loaded.torqueB2, sizeof(loaded.torqueB2));
      (void)prefs.getBytes("tb3", loaded.torqueB3, sizeof(loaded.torqueB3));
      loaded.hoRatePct = prefs.getUChar("ho", 100u);
      loaded.burstMs = prefs.getUShort("burst", 1000u);
      loaded.pauseMs = prefs.getUShort("pause", 1500u);
      if (prefs.getBytesLength("modeH") == sizeof(loaded.modeH))
        (void)prefs.getBytes("modeH", &loaded.modeH, sizeof(loaded.modeH));
    }
    prefs.end();
  }
  configSanitizePure(loaded);
  portENTER_CRITICAL(&cfgMux);
  cfg = loaded;
  portEXIT_CRITICAL(&cfgMux);
  runtimeInvalidate(RUNTIME_RESET_BOOT);
  if (!found) (void)cfgSaveSnapshot(loaded);
}

static void cfgCommit(const Config &input, NagRuntimeResetReason reason) {
  Config value = input;
  configSanitizePure(value);
  const bool locked = canTxMutex &&
      xSemaphoreTake(canTxMutex, portMAX_DELAY) == pdTRUE;
  portENTER_CRITICAL(&cfgMux);
  cfg = value;
  portEXIT_CRITICAL(&cfgMux);
  runtimeInvalidate(reason);
  if (locked) xSemaphoreGive(canTxMutex);
  (void)cfgSaveSnapshot(value);
}

static bool cfgClearPersistent() {
  if (!prefs.begin("nag-sc", false)) return false;
  const bool ok = prefs.clear();
  prefs.end();
  return ok;
}
