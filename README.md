# Nag-killer ESP32-S3 V3

> ⚠️ Research / educational firmware only.
>
> This project interacts with a vehicle CAN bus. It is intended for controlled bench testing, code review, and research environments only. Do not use this on public roads or in any situation where unsafe behavior could put people or property at risk. You are responsible for your own testing, wiring, configuration, and local laws.

Update by shubh2294.
This update is based on the original `nag-killer` project and focuses on stability, error handling, dashboard reliability, CAN frame validation, and ESP32-S3 hardware adaptation.
---

## What This Update Changes

This update includes fixes and improvements around:

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

| Device                       | Can Transceiver                 | CAN RX / CAN TX   | Can Bus      | Power                     |
| ---------------------------- | ------------------------------- | ----------------- | ------------ | ------------------------- |
| ESP32-S3-WROOM-1             | SN65HVD230 3.3V module          | GPIO 4 / GPIO 5   | 500 kbps CAN | USB-C or stable 5V supply |
| AtomS3 Lite ESP32S3          | ATOMIC CANBus Base (CA-IS3050G) | GPIO 5 / GPIO 6   | 500 kbps CAN | USB-C or stable 5V supply |
| Waveshare ESP32-S3-RS485-CAN | SIT1050T                        | GPIO 15 / GPIO 16 | 500 kbps CAN | USB-C or 7-36V supply     |


### Pin Definitions

```cpp
#define CAN_RX_PIN 4
#define CAN_TX_PIN 5
```

### Basic Wiring for SN65HVD230

```text
ESP32-S3 GPIO 5  ->  SN65HVD230 TXD
ESP32-S3 GPIO 4  ->  SN65HVD230 RXD
ESP32-S3 3.3V    ->  SN65HVD230 VCC
ESP32-S3 GND     ->  SN65HVD230 GND

SN65HVD230 CANH  ->  Vehicle PARTY CANH
SN65HVD230 CANL  ->  Vehicle PARTY CANL
```

Note: Some vehicle CAN networks already include termination. Do not add termination blindly without understanding the bus you are connecting to.

---

## Dashboard Notes

The dashboard exposes a local WiFi/web interface for configuration and live status.

SSID: Setup-%%%%  
Password: 12345678
 
Common endpoints:

| Endpoint      | Method   | Purpose               |
| ------------- | -------- | --------------------- |
| `/`           | GET      | Main dashboard        |
| `/api/config` | GET      | Current configuration |
| `/api/stats`  | GET      | Live runtime stats    |
| `/api/update` | POST/GET | Update settings       |
| `/api/reset`  | POST/GET | Reset config          |

---

## CAN State Labels

Dashboard CAN state labels were corrected to match ESP-IDF TWAI state ordering:

| Value | State      |
| ----- | ---------- |
| 0     | Stopped    |
| 1     | Running    |
| 2     | Bus-off    |
| 3     | Recovering |

---

## Build Notes

This project is intended for the Arduino ESP32 environment.
Required libraries are standard Arduino/ESP32 libraries such as:

- WiFi
- WebServer
- Preferences
- ESP32 TWAI driver

---

## Testing Notes

- Confirmed working on Model Y HW4 Firmware 2026.20.6.1

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
