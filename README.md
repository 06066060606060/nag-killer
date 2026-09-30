# Nag-killer V3.8.2 ESP32-S3 

> ⚠️ Research / educational firmware only.
>
> This project interacts with a Tesla vehicle CAN bus. It is intended for controlled bench testing, code review, and research environments only.It sends signals directly to the controller, not a physical command to the steering wheel. Do not use this on public roads or in any situation where unsafe behavior could put people or property at risk. You are responsible for your own testing, wiring, configuration, and local laws.
---

## What Update 3.8.2 Changes 

- New mode H (Human like) by LP_YL 
- OTA Update 
- New dashboard design 
- vehicule profiles 

---

## Hardware Target

This fork was adapted for:

| Device                       | Can Transceiver                 | CAN RX / CAN TX   | Can Bus      | Power                     |
| ---------------------------- | ------------------------------- | ----------------- | ------------ | ------------------------- |
| ESP32-S3-WROOM-1             | SN65HVD230 3.3V module          | GPIO 4 / GPIO 5   | 500 kbps CAN | USB-C or stable 5V supply |
| AtomS3 Lite ESP32S3          | ATOMIC CANBus Base (CA-IS3050G) | GPIO 6 / GPIO 5   | 500 kbps CAN | USB-C or stable 5V supply |
| Waveshare ESP32-S3-RS485-CAN | SIT1050T                        | GPIO 16 / GPIO 15 | 500 kbps CAN | USB-C or 7-36V supply     |

## Vehicle profiles 

The profile must be selected before NAG injection can be enabled. It is retained 
for configuration compatibility and labeling, but it no longer chooses the DAS 
frame ID. 

At each reboot the ESP32 passively observes both `0x399` and `0x39B`. An ID is 
locked only after three DLC-8 frames have a sequential 4-bit DAS counter at the 
expected 500 ms cadence. The first qualified ID remains locked until reboot; 
the other ID is ignored even if the locked source later becomes stale. 

Vehicle speed is read from Party CAN frame `0x257`. EPAS source and injected frames use `0x370`. 

## Modes and safety behavior 

- Modes A, B, and C preserve the uploaded source behavior with bounded settings. 
- Mode H uses the Rev4 human-interaction scheduler, Visual Rescue, speed gating, and a final hard torque cap of ±1.80 Nm. 
- `AP-Only Injection` defaults ON. Selecting `Legacy Y` or `Legacy 3` forces it OFF and disables the dashboard control because those single-CAN installations do not provide a reliable AP authorization state. `Highland / Juniper` keeps the control available. 
- The master ON/OFF choice, mode, profile, and settings are stored in the dedicated `nag-sc` preferences namespace. 
- A saved ON state is not transmitted immediately after reboot. Required CAN inputs must remain valid for 3 seconds before injection becomes effective. Legacy profiles do not require DAS for this countdown; `Highland / Juniper` does. AP engagement is not part of the restore countdown. 
- Legacy profiles also omit the live DAS/AP authorization gates, matching the proven standalone Legacy behavior while retaining EPAS, Hands-On, CAN, OTA, torque, and Mode H speed gates. Without DAS, Mode H Visual Rescue receives no warning transition and therefore does not trigger. 
- Once restoration completes, the READY latch remains set until reboot. A transient authorization loss blocks transmission through the applicable live safety gate without restarting the 3-second countdown. 
- On `Highland / Juniper`, if the locked DAS source disappears, transmission remains blocked until that same ID returns. The firmware never changes DAS IDs without a reboot. 
- Profile, mode, AP-gate, CAN recovery, OTA, and factory-reset changes invalidate in-flight runtime decisions before later transmissions. 
- Factory reset clears only this firmware's `nag-sc` preferences and restores profile unset, master OFF, and AP-only ON. 

## Dashboard and API 

Dashboard sim:

Connect to the device AP and open `192.168.4.1`. 

| Endpoint | Method | Purpose | 
| --- | --- | --- | 
| `/api/config` | GET | Saved controls and tuning | 
| `/api/status` | GET | Live status and diagnostics | 
| `/api/nag` | POST | Master ON/OFF | 
| `/api/mode` | POST | Mode A/B/C/H | 
| `/api/profile` | POST | Vehicle profile | 
| `/api/settings` | POST | AP-only and tuning | 
| `/update` | POST | Firmware OTA | 
| `/api/restart` | POST | Restart | 
| `/api/factory-reset` | POST | Clear SC settings and restart | 

## Build and validation 

Open the `nag-killer-v3.8.2-SC` folder in Arduino IDE and select the matching ESP32-S3 board. The sketch folder and `.ino` filename need to match. 

Host verification: 

```sh 
python3 tools/run_host_tests.py 
``` 

See `VALIDATION.md` for the exact verification record and remaining hardware checks. 

## Dashboard Notes 

The dashboard exposes a local WiFi/web interface for configuration and live status. 

SSID: NAG-KILLER-XXXX 
Password: 12345678


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

## Credits

- Original project: `@nicolozak` https://gitlab.com/nicolozak/nag-killer
- `Ev Open Can Mod` https://github.com/ev-open-can-tools/ev-open-can-tools
- Updated by X₿mod & LP_YL.
- ESP32 TWAI driver by Espressif Systems
- Automotive CAN research community

- ## Discord server: 
https://discord.gg/euPbYG8Npc

> **Support the project:**

<a href="https://www.buymeacoffee.com/xbmod" target="_blank"><img src="https://cdn.buymeacoffee.com/buttons/v2/default-yellow.png" alt="Buy Me a Coffee" style="height: 60px !important;width: 217px !important;" ></a>

  ---






