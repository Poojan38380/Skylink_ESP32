# Developer & AI Agent Handoff

High-signal notes for anyone modifying Skylink's SITL bridge, dashboard, or docs.  
**Verified stack:** WSL2 `sim_vehicle.py -v ArduCopter` + ESP32 `SITL_MODE` + Mission Planner TCP 5762 + dashboard WebSocket.

---

## Architecture (current)

```
Browser (GCS UI)  --WebSocket/json-->  ESP32 (web_server + flight_controller)
                                           |
                                           +-- TCP MAVLink --> ArduPilot SITL :5763 (SERIAL2)
Mission Planner   -- TCP :5762 ---------->  ArduPilot SITL (SERIAL1)
MavProxy          -- TCP :5760 ---------->  ArduPilot SITL (SERIAL0)
```

- **SITL host IP:** Set when a browser connects via WebSocket (`client->remoteIP()`). Must be the LAN IP of the PC running WSL, not `127.0.0.1` (ESP32 cannot reach Windows loopback as SITL).
- **Build flag:** `SITL_MODE` in `platformio.ini` selects `WiFiClient` TCP vs `Serial2` UART for Pixhawk.

---

## What works (keep these)

### 1. FreeRTOS mutex (`fcMutex`)
WebSocket runs on AsyncTCP thread; `loop()` runs flight_controller. All public FC methods take the mutex. **Never** call `arm()` from inside another locked path without using `sendArmDisarm()` internally.

### 2. `REQUEST_DATA_STREAM` @ 4 Hz
Secondary serial (5763) does not stream attitude/GPS/battery by default. Re-request every 10s in `handle()`.

### 3. GUIDED + autonomous MAVLink
- `SET_FLIGHT_MODE` → `MAV_CMD_DO_SET_MODE`
- `TAKEOFF` → `MAV_CMD_NAV_TAKEOFF`
- Avoid RC_OVERRIDE takeoff over WiFi.

### 4. Dynamic SITL host
`setSITLHost()` rejects loopback (`127.0.0.1`, `0.0.0.0`). Valid hosts come from dashboard WebSocket connect.

### 5. Port separation
| Port | Client |
|------|--------|
| 5762 | Mission Planner |
| 5763 | ESP32 only |

---

## Pitfalls (do not repeat)

### 1. `--out=tcpin:0.0.0.0:5763`
MavProxy listens on 5763; ArduCopter also binds SERIAL2 on 5763 → **Address already in use**, MavProxy gets **Connection refused** on 5760.  
**Fix:** `sim_vehicle.py -v ArduCopter` with no tcpin on 5763.

### 2. STABILIZE arming from web
Ground disarm without continuous RC. Always GUIDED for web arm/takeoff.

### 3. Telemetry overwriting UI mode selector
Never set `<select id="mode-select">` from incoming `flight_mode` telemetry.

### 4. `upload` without `uploadfs`
`data/index.html` lives in LittleFS. UI changes need `uploadfs`.

### 5. Old firmware symptoms
- Log shows port **5762** instead of **5763**
- `Unknown WebSocket command: SET_FLIGHT_MODE`
→ Re-flash `uploadfs` + `upload`.

### 6. Opening dashboard from wrong device
`remoteIP` must be the machine running SITL. Phone on same WiFi but different subnet may set wrong host.

---

## WebSocket command contract

See [README.md](./README.md#websocket-commands-dashboard--esp32). Adding commands: implement in `web_server.cpp` `handleWebSocketMessage`, then `flight_controller` API with mutex.

---

## Files to touch for common tasks

| Task | Files |
|------|-------|
| New MAVLink command | `flight_controller.h/cpp`, `web_server.cpp`, `data/index.html` |
| SITL port change | `flight_controller.h` (`sitlPort`), all docs in `docs/simulation/` |
| Hardware Pixhawk | `platformio.ini` (remove `SITL_MODE`), wire UART2 16/17 |
| WiFi networks | `data/wifi_networks.json` |
| Startup docs | `successful_run_guide.md`, this file |

---

## Success signals (regression test)

**ESP32 serial:**
```text
[INFO] Connected to ArduPilot SITL (TCP 5763)
[INFO] Setting flight mode: 4
[INFO] Sending command: ARM Drone
[INFO] Sending command: TAKEOFF to 5.00m
```

**Mission Planner:**
```text
Arming motors
```

**ArduCopter xterm:** No `bind failed on port 5763`.

---

## Recommendations for future agents

1. Read [successful_run_guide.md](./successful_run_guide.md) before editing startup flow.
2. After code changes, run `.venv\Scripts\platformio.exe run` and document any new ports/commands in `docs/simulation/README.md`.
3. Prefer GUIDED autonomous commands over RC_OVERRIDE for anything user-triggered from the web UI.
4. Physical hardware: comment `-D SITL_MODE`, configure Pixhawk `SERIAL2_PROTOCOL=2`, 115200 baud.
