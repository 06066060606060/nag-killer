# v3.8.2-SC Validation Record

Validation date: 2026-09-30 (Asia/Seoul)

## Source identity

- Input archive: `/Users/mezu/Downloads/nag-killer-main.zip`
- Input SHA-256: `d078d66d2f0369d625ec1d6253ed549e54c7f30a2df99d56163e2f6b781ecb24`
- Restore/dashboard implementation revision: `624a812`
- Firmware label: `v3.8.2-SC`

## Host verification

Canonical command: `python3 tools/run_host_tests.py`

The full suite was run twice from fresh temporary executable directories. Both runs reported:

- 7 C++ test executables passed with `-std=c++17 -Wall -Wextra -Werror`
- 13 Python contract tests passed
- no failures or warnings

C++ coverage:

- generated gzip header compile and gzip magic
- configuration defaults, sanitization, mode transitions, and hard torque bounds
- EPAS `0x370` decode/compose, Hands-On handling, and echo checks
- Mode H Rev4 scheduling, carrier behavior, speed/Visual gates, and ±1.80 Nm final cap
- NAG safety decisions, profile-aware DAS restore/live gates, forced Legacy AP-only OFF behavior, AP-independent 3-second safe restore, and the reboot-scoped READY latch
- automatic `0x399`/`0x39B` DAS qualification, permanent boot-session locking, and Party speed decoding

Python contract coverage:

- single-CAN runtime gates, transmit mutex, and epoch recheck
- dedicated `nag-sc` persistence and reset boundaries
- validated dashboard API mutations
- English status-first dashboard contents and exclusions
- deterministic dashboard gzip generation
- boot order, exact firmware version, pins, OTA boundary, and scoped factory reset
- source archive identity

## Dashboard determinism

`python3 tools/build_dashboard.py` was run twice. Both generated `index_html.h` files had SHA-256:

`c878d045e9cc9494d051c6eb707fe91bfba1d95833c86cb9686b26a9e416e31b`

The generated header was also compiled by the host suite and its gzip payload was decompressed by the Python contract test.

The dashboard was refreshed from the visual language of the supplied v3.8 source while retaining only SC controls and API routes. It was rendered at a 390×844 mobile viewport: Home and Mode pages had no horizontal overflow, the floating navigation stayed 30 px above the viewport bottom, and page switching reset scroll position to zero. Light and system dark color rules are included.

## Topology and exclusion audit

Production `.h` and `.ino` files were searched directly.

- Fixed built-in TWAI pins: RX GPIO 6, TX GPIO 5
- Requested frame IDs only: Party speed `0x257`, auto-qualified DAS `0x399`/`0x39B`, EPAS `0x370`
- No MCP2515 or external CAN-controller implementation
- No dual-CAN router or user-editable CAN IDs
- No R79, Auto Blinker, NOA, or `DAS_behaviorType` implementation
- No selectable older Mode H revisions
- Profile selection is retained for configuration compatibility but does not control DAS ID selection
- The first DAS ID with three sequential DLC-8, 500 ms counter frames remains locked until reboot; no stale-time source switching is implemented

## ESP32-S3 target build

The target sketch compiled successfully with Arduino CLI 1.5.1, ESP32 core
3.3.12, and FQBN `esp32:esp32:esp32s3`:

- Program storage: 941,201 bytes of 1,310,720 bytes (71%)
- Global variables: 47,080 bytes of 327,680 bytes (14%)
- Remaining dynamic memory: 280,600 bytes

The generic ESP32S3 Dev Module profile verifies compilation, but the exact
flash/PSRAM/USB options must still be matched to the installed board before
flashing.

## Required hardware validation

Before any road use, validate on an isolated bench and then in a controlled private environment:

1. Compile with the exact intended ESP32-S3 Arduino board profile.
2. Verify no transmission during the 10-second boot delay, saved-OFF state, profile-unset state, stale-input states, OTA, and CAN recovery.
3. Verify saved-ON restoration requires 3 seconds of continuously valid CAN prerequisites and starts while AP is inactive. On Legacy profiles, verify the countdown and live injection do not depend on DAS; on Highland / Juniper, verify DAS remains required. Confirm transient authorization loss after READY does not restart the countdown.
4. Verify Legacy Y and Legacy 3 force AP-only OFF and disable its dashboard control. Verify Highland / Juniper retains selectable AP-only behavior and its fresh-DAS gate. Verify Mode H retains its speed gate, and verify Visual Rescue only when a valid DAS Visual source is present.
5. Verify Mode H output never exceeds ±1.80 Nm and exact self-echo suppression works on the installed transceiver.
6. Validate `Legacy Y`, `Legacy 3`, and `Highland / Juniper` individually on their matching vehicle hardware, including profile-specific DAS/AP policy, optional Legacy Visual telemetry, speed, and EPAS signal layout.
7. Test bus-off/unplug recovery, dashboard status accuracy, OTA failure handling, restart, and factory reset.

A host PASS or successful target compile is not a substitute for those vehicle-specific checks.
