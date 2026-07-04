# nag-killer ESP32-S3

> ⚠️ Research / educational firmware only.
>
> This project interacts with a vehicle CAN bus. It is intended for controlled bench testing, code review, and research environments only. Do not use this on public roads or in any situation where unsafe behavior could put people or property at risk. You are responsible for your own testing, wiring, configuration, and local laws.

This fork is based on the original `nag-killer` project and focuses on stability, error handling, dashboard reliability, CAN frame validation, and ESP32-S3 hardware adaptation.

The goal of this fork is not to blindly rewrite the original project. The goal is to make the code easier to test, easier to debug, and less likely to hang or silently fail during controlled CAN research.

---

## What This Fork Changes

This fork includes fixes and improvements around:

- CAN message length validation
- TWAI startup and failure handling
- Dashboard/API error handling
- Mode-change state reset logic
- No-CAN watchdog behavior
- ESP32-S3 + external CAN transceiver support
- Serial logging for TX success/failure
- Safer startup behavior before any injection logic is allowed to run

---

## Hardware Target

This fork was adapted for:

| Part | Details |
|---|---|
| Microcontroller | ESP32-S3-WROOM-1 | AtomS3 Lite ESP32S3 |
| CAN Transceiver | SN65HVD230 3.3V module | ATOMIC CANBus Base (CA-IS3050G) |
| CAN RX | GPIO 4 | GPIO 5 |
| CAN TX | GPIO 5 | GPIO 6 |
| CAN Bus | 500 kbps CAN |
| Power | USB-C or stable 5V supply |

### Pin Definitions

```cpp
#define CAN_RX_PIN 4
#define CAN_TX_PIN 5
```

### Basic Wiring

```text
ESP32-S3 GPIO 5  ->  SN65HVD230 TXD
ESP32-S3 GPIO 4  ->  SN65HVD230 RXD
ESP32-S3 3.3V    ->  SN65HVD230 VCC
ESP32-S3 GND     ->  SN65HVD230 GND

SN65HVD230 CANH  ->  Vehicle CANH
SN65HVD230 CANL  ->  Vehicle CANL
```

Note: Some vehicle CAN networks already include termination. Do not add termination blindly without understanding the bus you are connecting to.

---

## Major Fixes

### 1. Fixed `canTask` Brace / Loop Issue

The original code had a brace mismatch that broke the intended infinite loop structure inside `canTask`.

Fix:

- Removed the stray brace
- Restored the proper `for(;;)` loop structure
- Ensured `vTaskDelay(1)` remains inside the task loop

---

### 2. Added CAN DLC Validation

The original code accessed CAN frame bytes directly without first confirming the frame length.

Example risk:

```cpp
f.data[0]
f.data[1]
f.data[2]
...
f.data[7]
```

If a frame arrived with fewer than 8 bytes, the code could read invalid data.

Fix:

Added validation before processing frames that require 8 bytes:

```cpp
if (src.data_length_code < 8) return;
```

This was added to logic paths such as:

- `echoModified()`
- `decideInjection()`

---

### 3. Improved `isOurs` Collision Logic

The original check only compared one byte against the configured torque value.

That could miss cases where part of the 12-bit value differed.

Fix:

The check now compares the full 12-bit raw value:

```cpp
uint16_t cfgRaw = ((cfg.torqueB2[i] & 0x0F) << 8) | cfg.torqueB3[i];
```

This makes the self-frame detection logic more accurate.

---

### 4. Fixed Dashboard Null Reference Errors

The dashboard JavaScript expected fields from `/api/stats` that were not actually returned by that endpoint.

Example issue:

```javascript
s.enabled
s.mode
```

Those values belong to the config object, not the stats object.

Fix:

- Dashboard now uses `cfg.enabled` and `cfg.mode`
- Added null checks before accessing config data
- Added fetch failure handling
- Added retry behavior for config loading
- Added an `updateTimer` guard to prevent multiple intervals from stacking

---

### 5. Added Startup Injection Delay

A fixed startup delay was added before injection logic is allowed to run.

```cpp
static const unsigned long INJECTION_BOOT_DELAY_MS = 30000;
bool bootDelayPassed = (millis() - bootTime) >= INJECTION_BOOT_DELAY_MS;
```

This gives the ESP32 and CAN bus time to stabilize before any active behavior occurs.

---

### 6. Replaced Permanent Freezes With Recovery Behavior

The original code could freeze forever if TWAI initialization failed.

Old behavior:

```cpp
while (1) {
    delay(1000);
}
```

New behavior:

- Print the error
- Wait briefly
- Reboot the ESP32

This avoids leaving the device stuck in a dead state with no recovery path.

---

### 7. Added No-CAN Watchdog

If the firmware starts but receives no CAN traffic after a set period, it will reboot.

Purpose:

- Detect bad wiring
- Detect sleeping vehicle/bus
- Detect wrong CAN pins
- Detect transceiver issues
- Avoid silent failure

---

### 8. Added NVS Corruption Recovery

If NVS storage is corrupted, the firmware can erase and reinitialize the NVS partition instead of failing permanently.

This helps recover from bad stored config states.

---

### 9. Increased TWAI RX Queue

The RX queue was increased:

```cpp
g.rx_queue_len = 256;
```

The original queue was smaller and could miss frames on a busy CAN bus.

---

### 10. Added TX Verification Logging

Serial output now includes confirmation when transmit attempts succeed or fail.

Example:

```cpp
Serial.printf("[TX] id=0x%03X data=%02X... t=%.2fNm\n", ...);
Serial.printf("[TX FAIL] %s\n", esp_err_to_name(err));
```

This makes it easier to confirm whether the firmware is actually transmitting.

---

### 11. Reset Static State on Mode Change

Some static variables inside injection decision logic could carry stale state when switching modes.

Fix:

Mode changes now reset related state so the next mode starts cleanly.

This helps prevent sequence corruption when changing between operating modes during testing.

---

### 12. Improved HTTP Input Validation

HTTP update handlers now validate user input more strictly.

Improvements include:

- `strtol()` end-pointer checks
- Hex value validation
- CAN ID range checking
- Percentage range checking
- Error response on invalid input

---

## Files Changed

### `v2_dashboard.ino`

Main firmware logic.

Changes include:

- ESP32-S3 pin updates
- TWAI failure recovery
- CAN DLC validation
- 30-second startup delay
- No-CAN watchdog
- RX queue increase
- TX verification logging
- Full 12-bit `isOurs` comparison
- NVS recovery handling
- Mode-change state reset

### `index_html.ino`

Dashboard / web UI.

Changes include:

- Null-safe config handling
- Correct use of `/api/config` vs `/api/stats`
- Fixed CAN state label ordering
- Button disabling until config loads
- Fetch error handling
- Retry behavior
- `updateTimer` guard against duplicate intervals

### `README.md`

Updated project documentation for this fork.

---

## Dashboard Notes

The dashboard exposes a local WiFi/web interface for configuration and live status.

Common endpoints:

| Endpoint | Method | Purpose |
|---|---|---|
| `/` | GET | Main dashboard |
| `/api/config` | GET | Current configuration |
| `/api/stats` | GET | Live runtime stats |
| `/api/update` | POST/GET | Update settings |
| `/api/reset` | POST/GET | Reset config |

---

## CAN State Labels

Dashboard CAN state labels were corrected to match ESP-IDF TWAI state ordering:

| Value | State |
|---|---|
| 0 | Stopped |
| 1 | Running |
| 2 | Bus-off |
| 3 | Recovering |

---

## Build Notes

This project is intended for the Arduino ESP32 environment.

Suggested board:

```text
ESP32S3 Dev Module
```

Suggested monitor speed:

```text
115200
```

Required libraries are standard Arduino/ESP32 libraries such as:

- WiFi
- WebServer
- Preferences
- ESP32 TWAI driver

---

## Testing Notes

Before any live vehicle testing, validate behavior in the safest possible way:

- Confirm wiring
- Confirm CAN RX traffic first
- Confirm dashboard loads
- Confirm config API responds
- Confirm no-CAN watchdog behavior
- Confirm boot delay behavior
- Confirm TX logs only occur when expected
- Confirm the device recovers from unplugged CAN / bad bus conditions

Do not assume a successful compile means the system is safe.

---

## Known Limitations

- This fork is not a polished release
- This fork has not been broadly tested across vehicle variants
- CAN IDs and behavior may vary by platform, trim, firmware, and hardware revision
- Incorrect wiring or configuration may cause unexpected behavior
- Web dashboard behavior depends on the ESP32 being powered and responsive
- Recovery behavior is designed for testing convenience, not production safety certification

---

## Changelog

### ESP32-S3 Stability / Safety Review Branch

#### Critical Fixes

- Fixed brace mismatch in `canTask`
- Added DLC validation before reading CAN frame bytes
- Fixed self-frame detection to compare full 12-bit value
- Fixed dashboard null reference crashes
- Fixed config/stat object confusion in JavaScript

#### Reliability

- Added 30-second injection startup delay
- Replaced TWAI init freeze with reboot recovery
- Added no-CAN watchdog
- Added NVS corruption recovery
- Increased RX queue from 64 to 256
- Added transmit success/failure logging

#### Dashboard

- Added null checks
- Added fetch error handling
- Added config retry behavior
- Fixed CAN state label order
- Disabled buttons until config successfully loads
- Prevented multiple update timers from stacking

#### Hardware

- ESP32-S3-WROOM-1
- Documented SN65HVD230 wiring
- Set CAN RX/TX pins to GPIO 4 and GPIO 5

- AtomS3 lite ESP32-S3
- ATOMIC CANBus Base (CA-IS3050G)
- Set CAN RX/TX pins to GPIO 5 and GPIO 6

---

## Disclaimer

This software is provided as-is for educational and research purposes.

It interacts with vehicle CAN systems and may cause unexpected behavior if used incorrectly.

The author of this fork assumes no responsibility for damage, injury, legal issues, warranty issues, or misuse.

Do not use this software on public roads.

Do not use this software in a moving vehicle.

Do not use this software unless you understand the risks of automotive CAN bus modification.

---

## Credits

- Original project: `06066060606060/nag-killer`
- ESP32 TWAI driver by Espressif Systems
- Automotive CAN research community
