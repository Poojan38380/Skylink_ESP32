# Skylink SITL Simulation — Documentation Index

**Status:** Verified working on Poojan's Asus Vivobook (WSL2 + Windows). WebSocket table updated May 2026.

This folder is the **single source of truth** for starting, operating, and extending the Skylink flight stack. AI agents should read documents in the order below before changing code.

---

## Read order (humans & AI agents)

| # | Document | Purpose |
|---|----------|---------|
| 1 | [successful_run_guide.md](./successful_run_guide.md) | **Start here.** Exact commands, sequence, success logs, troubleshooting |
| 2 | [agent_handoff.md](./agent_handoff.md) | Architecture lessons, pitfalls, code conventions |
| 3 | [start_simulation.md](./start_simulation.md) | Short Vivobook-specific quick start |
| 4 | [simulation_test_plan.md](./simulation_test_plan.md) | Full integration spec, WSL mirrored networking setup, Pixhawk migration |
| — | **[upgrade/](./upgrade/)** | **Post-MVP GCS roadmap** (map-first, relative controls, LED) |

---

## System at a glance

```
┌─────────────────┐     WebSocket      ┌──────────────┐     TCP :5763      ┌──────────────────┐
│  Chrome GCS     │ ◄──────────────► │   ESP32      │ ◄────────────────► │  ArduPilot SITL  │
│  (dashboard)    │   /ws JSON       │  Skylink     │   MAVLink          │  (WSL Ubuntu)    │
└─────────────────┘                  └──────────────┘                    └────────┬─────────┘
        │                                    │                                      │
        │ sets SITL host = browser PC IP     │ WiFi LAN                             │ SERIAL0 :5760
        └────────────────────────────────────┘                                      ▼
                                                                              ┌──────────────┐
┌─────────────────┐     TCP :5762                                           │  MavProxy    │
│ Mission Planner │ ◄──────────────────────────────────────────────────────│  (optional)  │
│  (Windows)      │     SERIAL1                                             └──────────────┘
└─────────────────┘
```

---

## Golden rules (do not break)

1. **SITL command:** `sim_vehicle.py -v ArduCopter` only — **never** `--out=tcpin:0.0.0.0:5763`.
2. **Ports:** MP → `5762` · ESP32 → `5763` · MavProxy → `5760`.
3. **Flash both:** `pio run --target uploadfs --target upload` when C++ or `data/` changes.
4. **Open dashboard before expecting SITL connect** — sets LAN IP from `client->remoteIP()`.
5. **Flight:** Always **GUIDED** → SET MODE → ARM → TAKEOFF (within ~10s of arm).

---

## Key source files

| File | Role |
|------|------|
| `include/flight_controller.h` | SITL TCP port 5763, MAVLink API, mutex |
| `src/flight_controller.cpp` | TCP client, streams, arm/takeoff/mode |
| `src/web_server.cpp` | WebSocket commands → flight controller |
| `data/index.html` | GCS dashboard UI |
| `platformio.ini` | `-D SITL_MODE` build flag |
| `data/wifi_networks.json` | Saved WiFi networks |

---

## WebSocket commands (dashboard → ESP32)

| Command | Payload | Effect | Phase |
|---------|---------|--------|-------|
| `SET_FLIGHT_MODE` | `{ "mode": "GUIDED" \| "STABILIZE" \| "LAND" \| "RTL" \| "LOITER" }` | `MAV_CMD_DO_SET_MODE` | 0 |
| `ARM_DRONE` | — | Arm motors | 0 |
| `DISARM_DRONE` | — | Disarm | 0 |
| `TAKEOFF` | `{ "altitude": 5 }` | `MAV_CMD_NAV_TAKEOFF` (meters) | 0 |
| `LAND` | — | LAND mode | 0 |
| `RTL` | — | RTL mode | 0 |
| `MOVE_BODY` | `{ "x": 5, "y": 0, "z": 0 }` | Body-relative offset (NED frame, meters). Requires GUIDED + armed + GPS 3D + AGL ≥ 2m | 4 |
| `YAW_RELATIVE` | `{ "deg": 90 }` | Relative yaw ±90° via `MAV_CMD_CONDITION_YAW` | 4 |
| `GOTO_LATLON` | `{ "lat": ..., "lon": ..., "alt": 5 }` | `SET_POSITION_TARGET_GLOBAL_INT`. Geofence checked in firmware (1000m radius) | 5 |
| `GOTO_ALT` | `{ "alt": 10 }` | Change altitude at current position | 5 |
| `LOITER_HERE` | — | Switch to LOITER mode | 5 |
| `RC_OVERRIDE` | `{ "roll": 1500, "pitch": 1500, "throttle": 1000, "yaw": 1500 }` | Direct RC channel override (PWM 1000–2000) | 5 |
| `LED_SET` | `{ "value": true }` | Set built-in LED on/off | 1 |
| `LED_TOGGLE` | — | Toggle built-in LED | 1 |
| `PING` | — | Returns PONG with round-trip latency | 0 |

**Preconditions for MOVE/YAW/GOTO:** Armed + GUIDED mode + GPS fix ≥ 3 + relative altitude ≥ 2m.

### Outbound events (ESP32 → browser)

| Event | Frequency | Content |
|-------|-----------|---------|
| `HEARTBEAT` | 5 Hz (200ms) | Telemetry: armed, alt, speed, GPS, battery, attitude, mode, STATUSTEXT, link status, build info |
| `ACK` | On each command ACK from ArduPilot | Command ID + result (ACCEPTED/DENIED/FAILED) |
| `PONG` | On PING | Round-trip timestamp |
| `ERROR` | On unknown command | Error message |
| `LED_STATE` | On LED change | Current LED state |

See [docs/simulation/upgrade/IMPLEMENTATION_HANDOFF.md](./upgrade/IMPLEMENTATION_HANDOFF.md) §4 for full detail.

---

## Improving the system (agent checklist)

- [ ] Create `esp32dev_hw` env (no SITL_MODE) for Pixhawk UART; keep `esp32dev_sitl` for daily dev
- [ ] Any new `flight_controller` public method must use `fcMutex`
- [ ] Never bind Mission Planner and ESP32 to the same UART/TCP port
- [ ] Do not overwrite UI mode selectors from live `flight_mode` telemetry
- [ ] Update this README if ports, commands, or startup sequence change
- [x] WebSocket command table expanded with Phase 4-5 commands (May 2026)
- [x] Outbound events documented (HEARTBEAT, ACK, PONG, ERROR, LED_STATE)
