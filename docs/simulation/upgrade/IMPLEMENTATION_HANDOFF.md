# GCS Upgrade — Implementation Handoff (for humans & AI agents)

**Purpose:** Single source of truth for what was built in the map-first GCS upgrade, what broke in real SITL runs, how it was fixed, and how to continue without repeating mistakes.

**Last verified state:** May 2026 — **FW build 12**, **FS build 15** (`include/skylink_config.h`, `data/skylink_build.json`, `data/gcs_config.js`).

**Read this before:** implementing Phase 6, changing telemetry rate, or touching MAVLink heartbeat parsing.

---

## 1. Executive summary

Skylink evolved from a monolithic `index.html` dashboard into a **tabbed, map-first GCS** served from ESP32 LittleFS, with firmware MAVLink bridging to ArduPilot SITL on TCP **5763**.

| Phases | Status |
|--------|--------|
| 0 — Code foundation | ✅ Done |
| 1 — Link & LED | ✅ Done |
| 2 — Map shell (Leaflet) | ✅ Done |
| 3 — Telemetry & preflight | ✅ Done |
| 4 — Relative body moves + keyboard | ✅ Done |
| 5 — Map click-to-fly, geofence, LOITER | ✅ Done (SITL acceptance tests still manual) |
| 6 — Polish & safety | ❌ Not started |
| 7+ — Missions / Pixhawk | 📋 Roadmap: [../PIXHAWK_HARDWARE_ROADMAP.md](../PIXHAWK_HARDWARE_ROADMAP.md) |

**Operational rule:** After **any** firmware change → `pio run --target upload`. After **any** `data/*` change → also `uploadfs`. Bump **both** build counters when you flash (see §8).

---

## 2. Architecture (where things live)

```
Browser (GCS PC)                    ESP32 (Skylink)                 SITL host (WSL/PC)
─────────────────                   ───────────────                 ─────────────────
index.html + gcs.css                web_server.cpp                  sim_vehicle.py
gcs.js (UI logic)        WS /ws      flight_controller.cpp    TCP   ArduCopter :5763
gcs_map.js (Leaflet)                main.cpp (telemetry loop)       (Mission Planner :5762)
gcs_config.js (UI tunables)         skylink_config.h (FW tunables)
```

- **Do not** use `webServerModule.handle()` — ESP **AsyncWebServer** is callback-driven.
- **SITL:** ESP connects **outbound** to the GCS machine IP on port **5763** when the browser opens `/ws` (client IP becomes SITL host).
- **Mission Planner** should use **5762** only — never share 5763 with MP or heartbeat noise returns.

Deeper layout: [ARCHITECTURE.md](./ARCHITECTURE.md). MAVLink mapping: [MAVLINK_COMMANDS.md](./MAVLINK_COMMANDS.md).

---

## 3. What was implemented (by phase)

### Phase 0 — Code foundation

- Split UI: `data/index.html`, `data/gcs.css`, `data/gcs.js`, `data/gcs_config.js`.
- `include/skylink_config.h` — firmware constants (single place).
- `include/ws_json.h` — serialize JSON to fixed buffer; `wsBroadcastJson()` skips send when no clients or queue full.
- `src/web_server.cpp` — `kCommands[]` dispatch table for WebSocket commands.
- Protocol version `"v": 1` on messages.
- `data/skylink_build.json` + heartbeat fields `fw_build`, `fs_build`, `fs_build_ok`.

**Deferred:** split `flight_controller.cpp` into `mavlink_telemetry.cpp` / `mavlink_commands.cpp`.

### Phase 1 — Link & LED

- `src/led_controller.cpp` + `src/main.cpp` `updateLinkLed()`:
  - WiFi down → LED off
  - WiFi up, no MAVLink → slow blink (500 ms)
  - Armed → fast blink (200 ms)
  - Otherwise solid
- Header **link chips**: WS, SITL, MAV, WiFi (RSSI).
- **SIMULATION MODE** banner when `SITL_MODE` build.
- `/health` JSON endpoint (uptime, builds, simulation flag).

### Phase 2 — Map-first shell

- Tabbed UI: **Map | Fly | Status | Log** (map is default).
- **Leaflet** vendored under `data/lib/leaflet/` (tiles still load from OSM URL unless offline tiles added).
- `data/gcs_map.js`:
  - Drone marker with yaw arrow
  - Home marker (`HOME_POSITION` or first 2D+ fix)
  - Trail polyline (60 points, client-only)
  - Follow / Center buttons
- Map dock: relative move pad (moved here from Fly tab in later work).

### Phase 3 — Telemetry & preflight

**Firmware (`flight_controller.cpp`):**

- Parse: `GLOBAL_POSITION_INT`, `HOME_POSITION`, `COMMAND_ACK`, `STATUSTEXT`.
- Ring buffer: 5 STATUSTEXT lines in heartbeat JSON.
- `SET_MESSAGE_INTERVAL` for attitude + global position streams.
- WebSocket telemetry **5 Hz** (200 ms) — reduced from 10 Hz after queue overflows (see §6).

**UI:**

- Preflight checklist (WiFi, GPS ≥3, MAVLink, battery, disarmed).
- Attitude bubble (roll/pitch).
- Flight mode **name** string (not raw mode number).
- ACK + STATUSTEXT → **Log** tab (not toasts).
- STATUSTEXT also in heartbeat → log via diff in `gcs.js` (`logNewStatusTexts`).

**Heartbeat flicker fix:** [HEARTBEAT_FLICKER_FIX.md](./HEARTBEAT_FLICKER_FIX.md) — filter vehicle-only HEARTBEAT; GCS heartbeat **sysid 255**; UI debounce 3 samples (`flightStateStableSamples`).

### Phase 4 — Relative moves

**WebSocket → firmware:**

| Command | Payload | MAVLink |
|---------|---------|---------|
| `MOVE_BODY` | `{ x, y, z }` metres, body NED | `SET_POSITION_TARGET_LOCAL_NED`, `MAV_FRAME_BODY_OFFSET_NED` |
| `YAW_RELATIVE` | `{ deg }` | `MAV_CMD_CONDITION_YAW` |

**Gating (firmware):** armed, **GUIDED**, GPS fix ≥ 3, AGL ≥ 2 m, `SKYLINK_CMD_DEBOUNCE_MS` (500 ms).

**UI:** Presets 1/3/5/10 m; custom **0.5–200 m**; keyboard (↑↓←→, Num 8/2/4/6); hints on buttons.

**Caps (current):** `SKYLINK_MOVE_BODY_MAX_M` = **200**, `SKYLINK_YAW_MAX_DEG` = **90**.

### Phase 5 — Map fly + loiter

**WebSocket → firmware:**

| Command | Payload | MAVLink / action |
|---------|---------|------------------|
| `GOTO_LATLON` | `{ lat, lon, alt }` alt relative to home | `SET_POSITION_TARGET_GLOBAL_INT`, `MAV_FRAME_GLOBAL_RELATIVE_ALT_INT` |
| `GOTO_ALT` | `{ alt }` | Same at current lat/lon |
| `LOITER_HERE` | — | Mode **LOITER** (5) |
| `SET_FLIGHT_MODE` | `{ mode }` | includes `LOITER` |

**Geofence:** haversine distance from **home** ≤ `SKYLINK_GOTO_MAX_RADIUS_M` (**1000 m** firmware + `geofenceRadiusM` in `gcs_config.js`). Map draws dashed circle.

**UI:**

- Map click → fly-here sheet (alt default = current relative alt + 5 m, clamp 2–50 m).
- Map overlay: **Loiter / Land / RTL**.
- Orange target marker on selected point.

**Note:** `GOTO_ALT` exists in firmware but has **no dedicated button** — use fly-here alt, body Up/Down (`MOVE_BODY` z), or add UI in Phase 6.

---

## 4. WebSocket command reference (implemented)

Inbound (browser → ESP), JSON text:

```json
{ "command": "MOVE_BODY", "x": 5, "y": 0, "z": 0 }
```

| `command` | Extra fields |
|-----------|----------------|
| `SET_FLIGHT_MODE` | `mode`: `GUIDED`, `LOITER`, `STABILIZE`, `LAND`, `RTL` |
| `ARM_DRONE` / `DISARM_DRONE` | — |
| `TAKEOFF` | `altitude` (metres) |
| `LAND` / `RTL` | — |
| `MOVE_BODY` | `x`, `y`, `z` |
| `YAW_RELATIVE` | `deg` |
| `GOTO_LATLON` | `lat`, `lon`, `alt` |
| `GOTO_ALT` | `alt` |
| `LOITER_HERE` | — |
| `PING` | — |
| `LED_SET` / `LED_TOGGLE` | optional `value` |

Outbound events: `HEARTBEAT` (telemetry bundle), `ACK`, `PONG`, `ERROR`, `LED_STATE`.

Full field list in heartbeat: see `WebServerModule::sendHeartbeat()` in `src/web_server.cpp`.

---

## 5. UI behaviour agents must know

### Tabs & move controls

- Relative moves and keyboard work on **Map** tab only (`activeTab === 'map'`).
- Moves require **armed + GUIDED + 3D GPS + ≥ 2 m** — `updateMoveControls()` enables `.move-dir` and `.btn-yaw`.
- **Custom distance:** reads from `#move-dist-custom` automatically in `getMoveDistanceM()` (0.5–200 m). **Apply** is optional. Click outside input before arrow keys (typing mode blocks shortcuts).

### Map click-to-fly

- Same gating as moves for opening fly-here sheet.
- Client **and** firmware both check geofence radius.
- Requires `home_valid` / home marker for distance check.

### Reconnect

- `gcs.js` exponential backoff reconnect; sets `ws = null` on close; avoids duplicate sockets if already `OPEN`/`CONNECTING`.

### Build mismatch

- Dashboard shows FS/FW tags; `fs_build_ok: false` means run **`uploadfs`**.

---

## 6. Critical incidents & fixes (read before changing WS or HB rate)

### 6.1 WebSocket queue overflow (“Too many messages queued”)

**Symptoms (serial monitor):**

```
[E][AsyncWebSocket.cpp:421] _queueMessage(): Too many messages queued: closing connection
[INFO] WebSocket client #2 connected from ...
[E][WiFiClient.cpp:429] write(): fail on fd 49, errno: 11
```

**Cause:** 10 Hz large `HEARTBEAT` JSON (~1 KB) + duplicate `STATUSTEXT` WS events + `COMMAND_ACK` events filled per-client queue (default 32). Server **closed** socket → browser reconnected → spiral. SITL TCP writes then failed with **EAGAIN** (errno 11) under load.

**Fixes applied:**

| Change | Location |
|--------|----------|
| Telemetry **5 Hz** (200 ms) | `SKYLINK_WS_TELEMETRY_INTERVAL_MS` |
| No WS send if `ws.count() == 0` | `main.cpp`, `sendHeartbeat`, `sendPendingFcEvents` |
| STATUSTEXT only in heartbeat, not separate WS flood | `pushStatusLine()` no longer `pushEvent` for text |
| Max **3** ACK events per loop | `SKYLINK_WS_MAX_EVENTS_PER_LOOP` |
| `wsBroadcastJson` checks `availableForWriteAll()` | `ws_json.h` |
| On connect: `setCloseClientOnQueueFull(false)` — drop frames instead of closing | `web_server.cpp` `WS_EVT_CONNECT` |
| `WS_MAX_QUEUED_MESSAGES=64` in `platformio.ini` | build_flags |

**Do not** revert to 10 Hz without measuring queue depth or shrinking heartbeat JSON.

**Library trap:** ESP Async WebServer **3.0.6** has `AsyncWebSocket("/ws")` only — **no** `AsyncWebSocket(url, maxClients, queueLen)` constructor. Queue size is compile flag `WS_MAX_QUEUED_MESSAGES` in `platformio.ini`.

### 6.2 Heartbeat flicker (GUIDED ↔ STABILIZE)

Documented in [HEARTBEAT_FLICKER_FIX.md](./HEARTBEAT_FLICKER_FIX.md). Parsing GCS heartbeats as vehicle state caused UI to disable move buttons intermittently.

### 6.3 Custom distance “not working”

**User expectation:** type distance → use buttons/keyboard.

**Old behaviour:** required clicking **Use**; `selectedMoveM` stayed on preset until then.

**Fix:** `getMoveDistanceM()` reads valid input field live; label updates on `input` event.

**Separate limits:** body moves **200 m** max; map geofence **1000 m**; fly-here alt **2–50 m** (`SKYLINK_GOTO_ALT_MAX_M`).

---

## 7. Current tunables (keep firmware + browser in sync)

| Constant | Value | Notes |
|----------|-------|-------|
| `SKYLINK_MOVE_BODY_MAX_M` | 200 m | Firmware clamps `MOVE_BODY` |
| `SKYLINK_GOTO_MAX_RADIUS_M` | 1000 m | Map click / `GOTO_LATLON` |
| `SKYLINK_GOTO_ALT_MAX_M` | 50 m | Fly-here / `GOTO_ALT` |
| `SKYLINK_MOVE_MIN_AGL_M` | 2 m | Moves & goto |
| `SKYLINK_YAW_MAX_DEG` | 90° | Per command |
| `SKYLINK_WS_TELEMETRY_INTERVAL_MS` | 200 ms | 5 Hz |
| `SKYLINK_CMD_DEBOUNCE_MS` | 500 ms | Per guided command |
| `SKYLINK_CMD_RATE_LIMIT_PER_SEC` | 2 | **Defined, not enforced yet** (Phase 6) |

Browser mirror: `data/gcs_config.js` (`moveMaxM`, `geofenceRadiusM`, etc.).

Authoritative table: [CONFIG_REFERENCE.md](./CONFIG_REFERENCE.md). Product defaults: [OPEN_DECISIONS.md](./OPEN_DECISIONS.md).

---

## 8. Build & flash discipline

Three places must agree for FS:

1. `SKYLINK_FS_BUILD` in `include/skylink_config.h`
2. `"fs"` in `data/skylink_build.json`
3. `fsBuild` in `data/gcs_config.js`

Firmware-only change: bump `SKYLINK_FIRMWARE_BUILD` only.

```bash
# From project root (venv with PlatformIO):
pio run --target upload          # firmware
pio run --target uploadfs        # LittleFS (HTML/JS/CSS)
pio run --target upload --target uploadfs   # both
```

**Common mistake:** editing `data/gcs.js` but only running `upload` → browser shows old UI with new firmware (or vice versa).

---

## 9. SITL checklist (unchanged ops)

1. Start SITL: `sim_vehicle.py -v ArduCopter` — **do not** bind Skylink port in sim_vehicle; MP uses **5762**, ESP uses **5763**.
2. Flash ESP with `SITL_MODE` (default in `platformio.ini`).
3. Open dashboard `http://<esp-ip>/` from the **same machine** that runs SITL (or ensure WSL mirrored networking so ESP can reach host IP).
4. WebSocket connect sets SITL host from client IP.
5. Fly tab: GUIDED → arm → takeoff before map moves / click-to-fly.

See [../start_simulation.md](../start_simulation.md) and [../successful_run_guide.md](../successful_run_guide.md).

---

## 10. File touch map (quick index)

| Area | Primary files |
|------|----------------|
| FW config | `include/skylink_config.h` |
| MAVLink / SITL | `src/flight_controller.cpp`, `include/flight_controller.h` |
| WebSocket / HTTP | `src/web_server.cpp`, `include/web_server.h`, `include/ws_json.h` |
| Telemetry loop | `src/main.cpp` |
| LED | `src/led_controller.cpp`, `src/main.cpp` |
| UI shell | `data/index.html`, `data/gcs.css` |
| UI logic | `data/gcs.js` |
| Map | `data/gcs_map.js`, `data/lib/leaflet/*` |
| UI config | `data/gcs_config.js` |
| Build flags | `platformio.ini` |

---

## What is NOT done (Phase 6 — next agent)

From [GCS_UPGRADE_ROADMAP.md](./GCS_UPGRADE_ROADMAP.md):

- [ ] **Hold-to-arm** 1.5 s (`armHoldMs` in `gcs_config.js` only today).
- [ ] **Command rate limiter** in firmware (`SKYLINK_CMD_RATE_LIMIT_PER_SEC` unused).
- [ ] Unified `LINK_STATUS` event at 1 Hz (constant reserved).
- [ ] Collapsible / filtered comms log; command history in `sessionStorage`.
- [ ] Update `successful_run_guide.md` and simulation README WebSocket table.
- [ ] Full regression script + 10 min heap-stability check.
- [ ] Optional: `GOTO_ALT` UI, vendored offline map tiles, structured error codes (`ERR_NOT_ARMED`).

**Bugs fixed May 26, 2026 (see PROJECT_ANALYSIS_AND_ORDERING_GUIDE.md):**
- [x] MOVE_BODY Z-axis safety — firmware rejects descent below 2m AGL
- [x] altitude/relative_alt separation — barometric altitude no longer overwritten by GPS relative altitude
- [x] SITL battery preflight bypass — unknown battery (0%) no longer blocks arming in simulation

Success criteria still open: preflight “gates” arming in UI only (not firmware lockout); arm failures via STATUSTEXT in log (works if MP/SITL sends text).

---

## 12. Lessons learned / mistakes to avoid

### For AI agents

1. **Read the library version before using constructors.** Assuming `AsyncWebSocket(url, 4, 64)` broke the build; v3.0.6 only accepts URL. Use `platformio.ini` `-D WS_MAX_QUEUED_MESSAGES=64` instead.

2. **Never flood AsyncWebSocket.** One fat periodic message + event per ACK + event per STATUSTEXT at 10 Hz will disconnect clients on ESP32. Prefer: lower rate, coalesce events, skip send when no client, check `availableForWriteAll()`.

3. **Filter MAVLink HEARTBEAT by sysid/type** before updating armed/mode. GCS heartbeats on the same TCP port look like disarmed STABILIZE.

4. **Keep MP on 5762 and Skylink on 5763.** Sharing the port reintroduces flicker and confusing mode changes.

5. **Bump FW and FS builds** when changing `data/*` or `skylink_config.h`. User sees mismatch in header chips.

6. **Align caps across three layers:** `skylink_config.h`, `gcs_config.js`, HTML `min`/`max`. We once had 100 m UI vs 20 m firmware vs 1000 m geofence — users thought custom distance was “broken”.

7. **UX: explicit “Apply” for custom numeric input failed user tests.** Read input at command time or on `input` event.

8. **`uploadfs` is required for JS/CSS/HTML** — agents often only run `upload`.

9. **Do not add heap `String` in hot paths** — project uses static JSON buffer in `ws_json.h` for a reason (see ARCHITECTURE.md).

10. **Relative moves need GUIDED** — if user is in LOITER after Phase 5, move pad stays disabled until they switch mode in Fly tab.

11. **Document acceptance tests as manual for SITL** until automated harness exists ([TESTING_STRATEGY.md](./TESTING_STRATEGY.md)).

### For humans

- errno 11 on `WiFiClient.write` during WS storms is **backpressure**, not “too many processes” — fix the WS flood first.
- Large single-step moves (200 m) may be rejected by ArduPilot params (`WPNAV_*`, etc.) even when Skylink allows them — check SITL logs / STATUSTEXT.

---

## 13. Suggested prompt for the next agent

```text
Continue Skylink GCS upgrade from docs/simulation/upgrade/IMPLEMENTATION_HANDOFF.md.
Implement Phase 6 from GCS_UPGRADE_ROADMAP.md (hold-to-arm, command rate limiter, log polish, ops docs).
Do not increase WS telemetry rate above 5 Hz without queue safeguards.
Follow ARCHITECTURE.md. Bump SKYLINK_FIRMWARE_BUILD and SKYLINK_FS_BUILD when flashing.
Test on SITL: arm → takeoff → MOVE_BODY → map GOTO_LATLON → LOITER → RTL.
```

---

## 14. Related documents

| Doc | Use when |
|-----|----------|
| [GCS_UPGRADE_ROADMAP.md](./GCS_UPGRADE_ROADMAP.md) | Phase checklist & acceptance |
| [HEARTBEAT_FLICKER_FIX.md](./HEARTBEAT_FLICKER_FIX.md) | Mode/arm UI flicker |
| [MAVLINK_COMMANDS.md](./MAVLINK_COMMANDS.md) | Adding MAVLink features |
| [CONFIG_REFERENCE.md](./CONFIG_REFERENCE.md) | Changing constants |
| [TESTING_STRATEGY.md](./TESTING_STRATEGY.md) | Test pyramid / future automation |
| [OPEN_DECISIONS.md](./OPEN_DECISIONS.md) | Locked product defaults |

---

*Maintainer: update this file when completing Phase 6 or changing WS/MAVLink behaviour.*
