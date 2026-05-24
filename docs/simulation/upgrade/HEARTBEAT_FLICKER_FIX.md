# HEARTBEAT flicker — cause and fix

## Symptom

Dashboard alternates **GUIDED ↔ STABILIZE** and **ARMED ↔ DISARMED** while the copter hovers, so move buttons and keyboard enable/disable rapidly.

## Root cause

`flight_controller.cpp` updated `telemetry.armed` and `telemetry.flight_mode` from **every** `HEARTBEAT` on the TCP link.

On SERIAL2 (port 5763) you receive:

| Source | sysid | type | custom_mode | armed bit |
|--------|-------|------|-------------|-----------|
| ArduCopter vehicle | 1 | QUADROTOR | 4 = GUIDED | set when armed |
| Skylink GCS (ours) | was **1** | GCS | 0 = STABILIZE | usually clear |
| Mission Planner (if routed) | 255 | GCS | 0 | clear |

GCS heartbeats are **not** the vehicle state. Mixing them at 5–10 Hz looks like mode/arm flicker in the UI.

## Fix (firmware)

1. **Filter inbound HEARTBEAT** — only `sysid==1`, `compid==1`, vehicle `type`, not `MAV_TYPE_GCS`.
2. **Send GCS heartbeat as sysid 255** — do not use sysid 1 (that looks like the autopilot).

## Fix (UI, secondary)

Debounce armed/mode for move controls: require **3** identical heartbeats (~300 ms at 10 Hz) before toggling `flightUiState` (`gcs_config.js` → `flightStateStableSamples`).

## If flicker persists

- Close Mission Planner or ensure it uses **5762** only, not the Skylink 5763 port.
- Check Serial monitor for `Setting flight mode` spam (real mode changes from FC).
- ArduPilot on bench: STABILIZE + throttle can auto-disarm on ground — that is real, not parse noise.
