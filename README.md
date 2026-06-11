# Port of the nag-killer by @nicolozak for other esp32 board
[@nicolozak](https://gitlab.com/nicolozak/nag-killer/-/tree/main?ref_type=heads)

# nag_echo v2 — runtime-configurable, with on-board WiFi dashboard

TESTED ON MODEL Y 2024 HW4 Firmware 2026.20 (Party CAN pin 2-3)

Same idea as `v1_simple`, but every parameter that v1 had hard-coded is
now changeable at runtime from a tiny web dashboard hosted by the ESP32
itself. No internet required — the board runs its own WiFi access point.

## What's different from v1

| | v1_simple | v2_dashboard |
|---|---|---|
| Target CAN ID | hard-coded `0x370` | runtime, default `0x370` |
| Torque values | fixed `+1.80 Nm` | table of 1–8, runtime |
| handsOn=1 rate | always | runtime, 0–100 % |
| Configuration | recompile + reflash | web UI, persisted in NVS |
| Dependencies | none beyond TWAI | `WiFi.h` + `WebServer.h` (built-in) |
| Binary size | ~280 KB | ~970 KB |
| Idle current | low | +~80 mA (WiFi AP) |
| Modes | one | **A**, **B**, **C**, **Custom** |

## Modes (one click in the dashboard)

### A — Simple
CAN `0x370`, fixed `+1.80 Nm`, `handsOn=1` on every echoed frame. Same
algorithm as v1. Proven on Model Y 2022 HW3 (pre-Juniper firmware).

### B — TSL6P (burst/pause)
CAN `0x052`, torque cycles through `{+1.80, +1.50, −1.50, −1.80}` Nm,
**bursty time pattern**: `1000 ms` of injection, `1500 ms` of rest by
default (both configurable). Mirrors the actual TSL6P device behaviour
observed in sniff logs — the rest periods are now believed to be the
real reason TSL6P avoids detection on stricter firmware (per @JNP's
re-analysis of the log).

> **Why bursty, not probabilistic?** v2's first cut applied
> `handsOn=1` on a fixed ~28 % of frames at random. Re-examination of
> the TSL6P log shows it's actually `~1 s on, ~1.4–2.0 s off` — the
> DAS-side detector is satisfied by the *rest period*, not by the
> per-frame probability. v2 now reproduces the time pattern.

### C — State machine (community algorithm by @Linu)

Implements the gated state machine from
[@Linu's STEERING_TORQUE_INJECTION_GUIDE](https://gitlab.com/nicolozak/nag-killer)
(posted in the upstream Discord thread). Watches
`DAS_autopilotHandsOnState` and only injects under tight conditions:

- `DAS_autopilotState ∈ {3,4,5,6}` (AP active range)
- `|SCCM_steeringAngle| ≤ 5°`
- `handsOnState == 1` → never inject (mandatory rest)
- `handsOnState == 2` → wait 2 s, then mild random-walk torque in the
  range `±0.5 … ±1.8 Nm`, **opposite** the current steering angle
- `handsOnState == 3` → wait 1 s, then sweep `−1.8 ↔ +1.8 Nm` in 1-s
  cycles (Linu's original spec is `±2.0`; we cap at `±1.80` per the
  pinned safety warning)

**Safety net:** if no fresh frames are seen on the configured
`apStateId` and `steeringId` within the last second, Mode C refuses
to inject. So if the default CAN IDs (`0x399`, `0x129`) don't match
your car's bus, the firmware safely no-ops instead of guessing.

The dashboard shows freshness indicators (`apState fresh` / `steering
fresh`) — both must be green for Mode C to work. If they stay red,
update the CAN IDs in the Mode C panel to match what your car actually
publishes.

### Custom
Whatever you typed into the advanced fields. The dashboard pre-fills
from whichever preset you last picked.

## Safety

- **Hard-clamp ±1.80 Nm in firmware** for every mode. The dashboard
  cannot exceed it. Honours the pinned Discord safety warning
  ("DO NOT GO OVER 1.8 Nm").
- Mode C refuses to inject without fresh context CAN frames.
- Going above 1.80 Nm has been reported to trigger unexpected FSD
  disengagements during turns. Don't try.

## Flash

```bash
arduino-cli compile --fqbn esp32:esp32:esp32 .
arduino-cli upload  --fqbn esp32:esp32:esp32 --port COMx .
```

The folder contains two `.ino` files — Arduino concatenates them
automatically.

## Use the dashboard

1. After flashing, the board prints the AP details on serial
   (`2 000 000` baud):
   ```
   AP: SSID=NagKiller-XXXX  PASS=nagkiller  IP=192.168.4.1
   ```
2. Connect a phone or laptop to that WiFi network.
3. Open `http://192.168.4.1` in any browser.
4. Pick **Mode A**, **B**, or **C** at the top, or fiddle with the
   advanced fields and hit **Apply all overrides**.

The dashboard polls `/api/stats` every 500 ms — you can watch the echo
counter climb in real time, see the live `handsOn` and torque readings,
and (for Mode C) the live `apState`, `handsOnState` and `steeringDeg`.

## REST API (for scripting)

| Method | Path | Purpose |
|---|---|---|
| `GET`  | `/api/config` | Current configuration |
| `GET`  | `/api/stats`  | Live stats incl. Mode C context |
| `POST` | `/api/mode?m=0` (or `m=1`, `m=2`) | Switch to Mode A / B / C |
| `POST` | `/api/update?...` | Patch any subset of fields. Query keys: `enabled`, `targetId`, `hoRatePct`, `burstMs`, `pauseMs`, `apStateId`, `steeringId`, `count`, `b2_<i>`, `b3_<i>` |
| `POST` | `/api/reset`  | Restore Mode A defaults, clear stats |

## Dual-core layout

- **Core 1**: TWAI receive + echo loop (priority 5). Same hot-path
  latency as v1.
- **Core 0**: WiFi AP + HTTP server (priority 1).

## License

GPL-3.0
