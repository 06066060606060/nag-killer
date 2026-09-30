# NAG KILLER v3.8.2-SC

Research firmware for standard Tesla vehicle architectures using one Party CAN connection. This build intentionally excludes the separate YL firmware family and its features.

> Safety warning: this firmware transmits CAN frames that affect steering-controller inputs. Use only for controlled bench research and private, supervised validation. Do not use on public roads. A successful compile or host test does not establish vehicle safety.

## Hardware and bus

- Target: ESP32-S3 with Arduino ESP32 core
- CAN controller: built-in ESP32 TWAI, 500 kbit/s
- RX: GPIO 6
- TX: GPIO 5
- Bus topology: one Party CAN connection
- Wi-Fi AP: `NAG-KILLER-XXXX`
- Password: `12345678`

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

[Dashboard view](https://06066060606060.github.io/nag-killer/)

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

Open the `nag-killer-v3.8.2-SC` folder in Arduino IDE and select the matching ESP32-S3 board. The sketch folder and `.ino` filename intentionally match.

Host verification:

```sh
python3 tools/run_host_tests.py
```

See `VALIDATION.md` for the exact verification record and remaining hardware checks.
