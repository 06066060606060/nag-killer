#include <Arduino.h>

#include "config_store.h"
#include "can_runtime.h"
#include "web_api.h"

#define FW_VERSION "v3.8.2-SC"

static constexpr uint32_t DRIVER_WAKE_DELAY_MS = 10000u;

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.printf("NAG KILLER %s\n", FW_VERSION);
  cfgLoad();
  if (xTaskCreatePinnedToCore(
          webTask, "web", 8192, nullptr, 2, nullptr, 0) != pdPASS) {
    Serial.println("Web task creation failed; local dashboard is unavailable.");
  }
  delay(DRIVER_WAKE_DELAY_MS);
  if (!canRuntimeBegin()) {
    Serial.println("TWAI initialization failed; NAG remains inhibited.");
    return;
  }
  if (xTaskCreatePinnedToCore(
          canRuntimeTask, "can", 8192, nullptr, 5, nullptr, 1) != pdPASS) {
    twaiReady = false;
    canRuntimeInvalidate(RUNTIME_RESET_CAN_RECOVERY);
    Serial.println("CAN task creation failed; NAG remains inhibited.");
  }
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}
