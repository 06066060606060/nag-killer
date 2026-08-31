# Nag-killer V3.1 ESP32-S3

> ⚠️ Research / educational firmware only.
>
> This project interacts with a Tesla vehicle CAN bus. It is intended for controlled bench testing, code review, and research environments only.It sends signals directly to the controller, not a physical command to the steering wheel. Do not use this on public roads or in any situation where unsafe behavior could put people or property at risk. You are responsible for your own testing, wiring, configuration, and local laws.
---

## What Update 3.1 Changes 

- New mode C (Random walk variation) by @wewe9v9v 
- OTA Update 
- New dashboard design 
- TWAI auto recovery 

---

## Hardware Target

This fork was adapted for:

| Device                       | Can Transceiver                 | CAN RX / CAN TX   | Can Bus      | Power                     |
| ---------------------------- | ------------------------------- | ----------------- | ------------ | ------------------------- |
| ESP32-S3-WROOM-1             | SN65HVD230 3.3V module          | GPIO 4 / GPIO 5   | 500 kbps CAN | USB-C or stable 5V supply |
| AtomS3 Lite ESP32S3          | ATOMIC CANBus Base (CA-IS3050G) | GPIO 6 / GPIO 5   | 500 kbps CAN | USB-C or stable 5V supply |
| Waveshare ESP32-S3-RS485-CAN | SIT1050T                        | GPIO 16 / GPIO 15 | 500 kbps CAN | USB-C or 7-36V supply     |


### Pin Definitions

```cpp
#define CAN_RX_PIN 4
#define CAN_TX_PIN 5
```

## Dashboard Notes

The dashboard exposes a local WiFi/web interface for configuration and live status.

SSID: Setup-XXXX  
Password: 12345678

## Modes (one click in the dashboard)

### A — Simple
CAN `0x370`, fixed `+1.80 Nm`, `handsOn=1` on every echoed frame. 

### B — TSL6P (burst/pause)
CAN `0x370`, torque cycles through `{+1.80, +1.50, −1.50, −1.80}` Nm,
**bursty time pattern**: `1000 ms` of injection, `1500 ms` of rest by
default (both configurable). Mirrors the actual TSL6P device behaviour
observed in sniff logs — the rest periods are now believed to be the
real reason TSL6P avoids detection on stricter firmware (per @JNP's
re-analysis of the log).

### C — Random Walk Variation
Add random walk variation in the injected torque values in order to evade any telemetry detection.
Always applies positive torque values (human like).
not inject if there is real hands on.
 
## Common endpoints

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

## Confirmed working
-  Tesla Model Y 2024 HW4 (EU) pin 2/3 (2026.20.6.1)
-  Tesla Model 3 Performance 2026 HW4 (US) pin 2/3
-  Tesla Model S 2017 HW3/MCU2 (US) pin 13/14
-  Tesla Model 3 LR AWD 2026 (EU) HW4 pin 2/3

## Know Bug
- can state displaying "recovering" even if everything is working correctly

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


## Variant
- Nag-killer by we9v9v HW3 FSD mode C 
https://github.com/we9v9v/nag-killer-9v-random/tree/main 

- Nag-killer & EU-Summon-Unlock unified for LilyGO/T-2Can 
https://github.com/06066060606060/T2CAN-Nag-killer-EU-unlock 

- PlatformIO Project by Hboop 
https://github.com/Hboop/nag-killer/tree/esp32s3-stability-safety-review 

## Credits

- Original project: `@nicolozak` https://gitlab.com/nicolozak/nag-killer
- `Ev Open Can Mod` https://github.com/ev-open-can-tools/ev-open-can-tools
- Updated by X₿mod & Hboop.
- ESP32 TWAI driver by Espressif Systems
- Automotive CAN research community

- ## Discord server: 
https://discord.gg/euPbYG8Npc

> **Support the project:**

<a href="https://www.buymeacoffee.com/xbmod" target="_blank"><img src="https://cdn.buymeacoffee.com/buttons/v2/default-yellow.png" alt="Buy Me a Coffee" style="height: 60px !important;width: 217px !important;" ></a>

Bitcoin: bc1pl9nuyhqd78gjc2wdcqr39de7qwtff732ngr28vy8r2sxfa7a6uzsrhe387  
Lightning: ₿cakegrip53@phoenixwallet.me


  ---
<img width="270" height="492" alt="Screenshot_2026-08-31-17-39-22-292_com microsoft emmx" src="https://github.com/user-attachments/assets/ecfb2f57-b0d4-4f1d-a895-c3813528b516" />





