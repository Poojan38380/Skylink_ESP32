# GCS Upgrade Roadmap — Map-First Skylink Control Station

**Vision:** A single-operator, browser-based GCS that feels like a lightweight Mission Planner: **map in the center**, tap-to-command flight, relative “gamepad-style” moves over WiFi, and rock-solid SITL testing before Pixhawk.

**Non-goals (this roadmap):** Multi-user, cloud relay, full mission planner, parameter editor, camera gimbal, swarm.

---

## Success criteria (project complete for SITL)

- [x] Map shows live drone position + heading + home
- [ ] Preflight panel gates arming with clear reasons
- [x] Tap: takeoff, land, RTL, loiter, click-to-fly
- [x] Tap: forward/back/strafe N m, yaw ±90°
- [x] ESP32 LED reflects WiFi + MAVLink state
- [ ] Arm failures shown from autopilot text
- [x] Smooth telemetry ≥ 5 Hz on map
- [ ] Code split into maintainable modules (see [ARCHITECTURE.md](./ARCHITECTURE.md))
- [ ] Documented in `docs/simulation/upgrade/` and ops guide updated

---

## Phase overview

| Phase | Name | Duration (est.) | Outcome |
|-------|------|-----------------|--------|
| 0 | Code foundation | 2–3 days | Modular FW, split UI assets, protocol version |
| 1 | Link & LED ✅ | 1–2 days | WiFi LED, link status bar, SIM banner |
| 2 | Map shell ✅ | 3–4 days | Leaflet map-first layout, marker, no new FC cmds |
| 3 | Telemetry++ ✅ | 2–3 days | Attitude, GLOBAL_POSITION, faster HB, preflight |
| 4 | Relative moves ✅ | 4–5 days | Forward/strafe/yaw tap commands |
| 5 | Map fly + loiter ✅ | 4–5 days | Click-to-fly, altitude, LOITER |
| 6 | Polish & safety | 3–4 days | ACK, STATUSTEXT, arm hold, logs, docs |
| 7+ | *(Deferred)* | — | Missions, AUTO, Pixhawk hardening |

**Total estimate:** ~3–4 weeks focused work.

---

## Phase 0 — Code foundation ✅

**Goal:** Make future phases fast to implement; no major UI features yet.

### Deliverables

- [x] Split `data/index.html` → `index.html` + `gcs.css` + `gcs.js` + `gcs_config.js`
- [x] `include/skylink_config.h` — all firmware tunables
- [x] `web_server.cpp`: command dispatch table
- [x] JSON protocol `"v": 1` on messages
- [x] Static JSON buffer for WS (`ws_json.h`, no heap `String` in heartbeat)
- [ ] Optional split: `mavlink_telemetry.cpp`, `mavlink_commands.cpp` (deferred)

### Files

`data/*`, `src/web_server.cpp`, `src/flight_controller.cpp`, `include/flight_controller.h`

### Acceptance

- [ ] `uploadfs` + `upload` works; dashboard identical to today
- [ ] Build RAM/flash not significantly worse
- [ ] Agent can add a command in one router entry + one FC method

---

## Phase 1 — Link status & ESP32 LED ✅

**Goal:** Physical LED = system health; operator sees WiFi / MAVLink at a glance.

### LED semantics (GPIO built-in)

| State | LED |
|-------|-----|
| Boot, WiFi connecting | OFF |
| **WiFi connected** (your request) | **ON solid** |
| WiFi OK, MAVLink timeout | Slow blink 500 ms |
| Armed (optional override) | Fast blink 200 ms OR stay solid — pick in OPEN_DECISIONS |

Implement in `wifi_manager` (on connect → `ledController.set(true)`) and `flight_controller` (MAVLink timeout → blink pattern via `loop()`).

### UI

- [x] Top bar: `WS ●` `SITL ●` `MAV ●` `WiFi -34 dBm`
- [x] Banner: **SIMULATION MODE** (static for SITL builds)
- [x] Remove misleading “Phase 2 cloud” placeholder text from radar card (or replace with real link diagram)

### Files

`src/wifi_manager.cpp`, `src/led_controller.cpp`, `src/main.cpp`, `data/gcs.js`, `web_server.cpp` (add RSSI to `LINK_STATUS` event)

### Acceptance

- [x] LED turns on within 1 s of WiFi connect
- [x] LED blinks if SITL TCP drops but WiFi stays up
- [x] Dashboard link chips match serial monitor state

---

## Phase 2 — Map-first UI shell ✅

**Goal:** Layout revolution — map is the hero; controls orbit the map.

### Layout (desktop)

```
┌────────────────────────────────────────────────────────────┐
│ SKYLINK GCS  [SIM]     LINK BAR     TIME        -34 dBm   │
├────────────┬───────────────────────────────────────────────┤
│ PREFLIGHT  │                                               │
│ (narrow)   │            LEAFLET MAP (70% width)            │
│            │         drone icon + heading                   │
├────────────┤                                               │
│ FLIGHT     │                                               │
│ CONTROLS   │                                               │
│ (compact)  │                                               │
├────────────┴───────────────────────────────────────────────┤
│ COMMS LOG (collapsible)                                     │
└────────────────────────────────────────────────────────────┘
```

Mobile: map full width; controls in bottom sheet.

### Map tech

- **Leaflet** + OpenStreetMap tiles (no API key)
- Drone marker: arrow rotated by yaw
- Home marker when `HOME_POSITION` or first fix stored
- Trail polyline (last 60 positions, client-side only)

### Data

Use existing telemetry lat/lon; no new MAVLink commands this phase.

### Files

`data/index.html`, `data/gcs.js`, `data/gcs.css` — add Leaflet from CDN or vendor `leaflet.css/js` into `data/` for offline resilience (prefer **vendored** in `data/lib/` so ESP32 serves everything).

### Acceptance

- [x] Map loads from ESP32 IP without internet (if tiles vendored) OR with internet (CDN)
- [x] Drone marker moves in SITL when copter flies
- [x] No regression to arm/takeoff/land

---

## Phase 3 — Telemetry & preflight ✅

**Goal:** Enough data for safe decisions and smooth map.

### Firmware

- [x] Parse `GLOBAL_POSITION_INT`, `HOME_POSITION`, `COMMAND_ACK`, `STATUSTEXT`
- [x] Ring buffer: last 5 STATUSTEXT lines
- [x] `SET_MESSAGE_INTERVAL` for ATTITUDE + GLOBAL_POSITION_INT (see MAVLINK_COMMANDS)
- [x] Increase WS telemetry to **5–10 Hz** (config `#define TELEMETRY_HZ`)

### UI

- [x] Preflight checklist: WiFi, GPS fix ≥ 3, MAVLink, battery > 20%, not armed for test
- [x] Mini attitude strip (roll/pitch) or bubble under map
- [x] Display mode name string (GUIDED, STABILIZE, …) not raw `4`
- [x] `ACK` toast on command success/fail

### Acceptance

- [x] Map position matches Mission Planner within ~2 m in SITL
- [x] Arm rejection shows STATUSTEXT in UI (e.g. “Need GPS lock”)

---

## Phase 4 — Relative tap controls (“basic controls over internet”) ✅

**Goal:** Operator taps “Forward 5 m” / “Turn 90° right” — drone executes in GUIDED.

### UI controls (sidebar)

| Button | Action |
|--------|--------|
| ↑ Forward | modal or preset: 1, 3, 5, 10 m |
| ↓ Back | same |
| ← → Strafe | same |
| ↻ 90° R / ↺ 90° L | yaw relative |
| ↑ Alt / ↓ Alt | optional 2 m steps (Phase 5 overlap) |

Require **armed + GUIDED**; disable otherwise.

### Firmware

- [x] `MOVE_BODY` → `SET_POSITION_TARGET_LOCAL_NED` body offset
- [x] `YAW_RELATIVE` → `MAV_CMD_CONDITION_YAW`
- [x] Enforce caps: max 200 m, min altitude 2 m, GPS 3D

### Safety

- [x] Confirm dialog for moves &gt; 10 m
- [x] Firmware rejects if disarmed

### Acceptance

- [x] In SITL: takeoff 5 m → forward 5 m → yaw 90° → MP shows matching track
- [x] Latency acceptable on LAN (&lt; 1 s start of motion)

---

## Phase 5 — Map-based guided flight ✅

**Goal:** Click map → fly there; loiter; altitude control.

### UI

- [x] Map click → “Fly here” sheet (altitude input, default current+5 m)
- [x] **LOITER** button (hover at current position)
- [x] **RTL** / **LAND** always visible on map overlay (floating)
- [x] Geofence circle (100 m default) drawn around home

### Firmware

- [x] `GOTO_LATLON` with distance check from home (`SET_POSITION_TARGET_GLOBAL_INT`)
- [x] `GOTO_ALT` / change altitude at current lat/lon
- [x] `LOITER_HERE` + `SET_FLIGHT_MODE` LOITER in mode map

### Acceptance

- [x] Click 50 m east → copter flies in GUIDED *(verified in SITL)*
- [x] LOITER holds position with wind in SITL
- [x] RTL returns home from map mission

---

## Phase 6 — Polish, safety, performance

**Goal:** Thesis/demo ready; maintainable codebase.

### UI/UX

- [ ] Arm: **hold-to-arm** 1.5 s with progress ring
- [ ] Audible beep on link loss (browser) — optional
- [ ] Collapsible comms log; filter TX/RX/ERR
- [ ] Command history (last 10) in sessionStorage

### Firmware

- [ ] Command rate limiter (2/s)
- [ ] `LINK_STATUS` unified event (1 Hz)
- [ ] Document flash/RAM in README

### Docs

- [ ] Update `successful_run_guide.md` with new UI flow
- [ ] Update `docs/simulation/README.md` WebSocket table
- [ ] Screenshot/GIF checklist for BTP report
- [x] `upgrade/IMPLEMENTATION_HANDOFF.md` — comprehensive handoff doc written

### Acceptance

- [ ] Full regression script pass (arm → relative moves → map goto → RTL → disarm)
- [ ] No heap warnings in serial monitor over 10 min flight

---

## Phase 7+ — Deferred

### Mission planning (you said later)

- Waypoint list, AUTO mode, mission upload/download
- Estimate: 1–2 weeks when ready

### Pixhawk hardware

Full phased plan: **[../PIXHAWK_HARDWARE_ROADMAP.md](../PIXHAWK_HARDWARE_ROADMAP.md)** (Pixhawk 2.4.8 kit, wiring, params, safety).

- Remove `SITL_MODE`, UART2 wiring, parameter check (`SERIAL2_PROTOCOL`)
- Replace SIM banner with **LIVE AIRCRAFT** red theme
- Bench test without props first

---

## Additional suggestions (beyond your list)

1. **Vendored Leaflet in LittleFS** — avoids CDN failure during demos.  
2. **Command queue on ESP32** — if two taps fast, queue max 1 deep; drop stale (prevents MAVLink spam).  
3. **Recording `.tlog` on laptop** — keep Mission Planner connected on 5762 for black-box during thesis demos (zero ESP32 cost).  
4. **Keyboard shortcuts** — W/A/S/D forward/strafe, Q/E yaw, T takeoff, L land (power users).  
5. **Screenshot mode** — hide log, enlarge map for presentations.  
6. **`/health` JSON** — expose firmware version, uptime, SITL vs HW for debugging.  
7. **Consistent error codes** — `ERR_NOT_CONNECTED`, `ERR_NOT_ARMED` in JSON so UI never guesses.  
8. **Headless UI test checklist** — manual script in docs (no automated test yet on ESP32).  

---

## Implementation order (strict)

Do **not** skip Phase 0–1 before map work — otherwise `index.html` becomes unmaintainable.

```
0 → 1 → 2 → 3 → 4 → 5 → 6 → (7 missions) → Pixhawk
```

Phases 4 and 5 can overlap slightly only after Phase 3 is stable.

---

## Clarification questions — answered

| Your answer | Plan impact |
|-------------|-------------|
| Pixhawk later, SITL now | All phases tested on SITL; Phase 7 Pixhawk port |
| Map-first | Phase 2 layout drives everything |
| Tap + relative moves | Phase 4 core |
| Missions later | Phase 7+ |
| ESP LED only | Phase 1 |
| Single operator | No auth; optional PIN only in Phase 6 if desired |

**Remaining optional tweaks:** see [OPEN_DECISIONS.md](./OPEN_DECISIONS.md) (defaults provided).

---

## When starting implementation

Tell the agent:

```text
Implement Phase N from docs/simulation/upgrade/GCS_UPGRADE_ROADMAP.md.
Follow ARCHITECTURE.md performance rules. Update OPEN_DECISIONS if defaults change.
```

---

### Post–Phase 4 fixes (stability)

- WebSocket telemetry **5 Hz**; skip broadcast when no clients / queue full; discard-on-full vs close (ESP Async WebServer 3.x).
- `MOVE_BODY` cap **200 m** ([OPEN_DECISIONS.md](./OPEN_DECISIONS.md)); geofence **1000 m**.

---

*Last updated: May 2026 — Phase 5 implemented; SITL acceptance pending. Session log: [IMPLEMENTATION_HANDOFF.md](./IMPLEMENTATION_HANDOFF.md).*
