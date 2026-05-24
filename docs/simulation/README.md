# Skylink SITL Simulation — Documentation Index

**Status:** Verified working on Poojan's Asus Vivobook (WSL2 + Windows), May 2026.

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

| Command | Payload | Effect |
|---------|---------|--------|
| `SET_FLIGHT_MODE` | `{ "mode": "GUIDED" \| "STABILIZE" \| "LAND" \| "RTL" }` | `MAV_CMD_DO_SET_MODE` |
| `ARM_DRONE` | — | Arm motors |
| `DISARM_DRONE` | — | Disarm |
| `TAKEOFF` | `{ "altitude": 5 }` | `MAV_CMD_NAV_TAKEOFF` (meters) |
| `LAND` | — | LAND mode |
| `RTL` | — | RTL mode |

---

## Improving the system (agent checklist)

- [ ] Comment out `-D SITL_MODE` in `platformio.ini` for real Pixhawk (UART2 GPIO16/17).
- [ ] Any new `flight_controller` public method must use `fcMutex`.
- [ ] Never bind Mission Planner and ESP32 to the same TCP port.
- [ ] Do not overwrite UI mode selectors from live `flight_mode` telemetry.
- [ ] Update this README if ports, commands, or startup sequence change.
