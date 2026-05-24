# GCS Upgrade — Architecture & Performance Rules

Goals: **fast**, **readable**, **scalable**, **safe**. Optimized for ESP32 constraints and single-operator WebSocket GCS.

---

## Layered design

```
┌─────────────────────────────────────────────────────────┐
│  data/gcs/          Browser (Leaflet map, UI state)     │
│  index.html, gcs.js, gcs.css                            │
└───────────────────────────┬─────────────────────────────┘
                            │ WebSocket JSON (text)
┌───────────────────────────▼─────────────────────────────┐
│  web_server.cpp     Parse commands, rate-limit, route     │
└───────────────────────────┬─────────────────────────────┘
                            │
┌───────────────────────────▼─────────────────────────────┐
│  flight_controller    MAVLink encode/decode, mutex      │
│  (+ optional mavlink_tx / telemetry modules later)      │
└───────────────────────────┬─────────────────────────────┘
                            │ TCP :5763 (SITL) or UART2 (Pixhawk)
┌───────────────────────────▼─────────────────────────────┐
│  ArduPilot                                                    │
└─────────────────────────────────────────────────────────┘

Parallel (non-FC):
  wifi_manager  →  led_controller (link semantics)
  logger        →  serial only, no heap alloc in hot path
```

---

## Firmware modularity (target layout)

| Module | Responsibility |
|--------|----------------|
| `flight_controller.cpp` | Connection lifecycle, heartbeat, stream requests |
| `mavlink_telemetry.cpp` *(new)* | `processMavlinkMessage`, `FCTelemetry` updates only |
| `mavlink_commands.cpp` *(new)* | arm, mode, takeoff, reposition, body-offset moves |
| `web_server.cpp` | WS events, JSON ↔ command dispatch, telemetry broadcast |
| `link_status.cpp` *(new, optional)* | Aggregate WiFi + MAVLink + armed into one struct |

Keep **public API** in `flight_controller.h` thin; implementation can split across `.cpp` files without changing callers.

---

## Performance rules (mandatory)

### ESP32 / C++

1. **No `String` concatenation in hot paths** (`handle()`, MAVLink RX loop). Use `logger` only for rare events or throttle logs (e.g. reconnect every 5 s).
2. **Mutex scope minimal** — never hold `fcMutex` across `sitlClient.connect()` or blocking I/O.
3. **Fixed buffers** — `uint8_t buf[MAVLINK_MAX_PACKET_LEN]` on stack; no `new`/`malloc` per packet.
4. **Telemetry snapshot** — copy `FCTelemetry` under mutex once per WS heartbeat; serialize outside lock.
5. **ArduinoJson** — use `JsonDocument` with explicit size (`json.h` doc recommends capacity); avoid `serializeJson` to `String` in loop; use static char buffer + `serializeJson(doc, buf, sizeof buf)`.
6. **WebSocket TX** — one compact HEARTBEAT JSON every **250–500 ms** (not 3 s if map needs smoothness); split **slow** status (1 s) vs **fast** pose (4–10 Hz) if bandwidth becomes an issue.
7. **Command debounce** — reject duplicate commands within 500 ms; max 2 cmd/s per client (see OPEN_DECISIONS).
8. **Parse only needed MAVLink IDs** — switch on `msgid`; do not forward full firehose to browser.

### Browser / LittleFS

1. **Split assets** — `gcs.js`, `gcs.css` cached by browser; smaller HTML shell.
2. **Map** — throttle marker updates to telemetry rate; don’t redraw full DOM on every HB.
3. **Use `requestAnimationFrame`** for attitude horizon if added.
4. **No heavy frameworks** — vanilla JS + Leaflet only (fits ESP32-served static files).

### Protocol

1. **Stable command schema** — `{ "type":"command", "command":"...", ... }` version field later: `"v":1`.
2. **Events** — `HEARTBEAT` (telemetry), `LINK_STATUS`, `ACK`, `STATUSTEXT`, `ERROR`.
3. **Do not send RC_OVERRIDE streams** from UI in this roadmap (latency unsafe); use GUIDED position/yaw commands only.

---

## WebSocket event schema (target)

### Client → ESP32 (commands)

Document every command in [MAVLINK_COMMANDS.md](./MAVLINK_COMMANDS.md) as implemented.

### ESP32 → Client (events)

| Event | Rate | Payload highlights |
|-------|------|-------------------|
| `HEARTBEAT` | 4–10 Hz | pose, battery, GPS, mode, armed |
| `LINK_STATUS` | 1 Hz | wifi rssi, sitl_connected, mav_active, led_state |
| `ACK` | on response | command id, result enum |
| `STATUSTEXT` | as received | severity, text (ring buffer 5) |
| `ERROR` | rare | human message |

---

## Safety architecture

1. **Preflight gating** — UI blocks ARM until checks pass; firmware optionally double-checks `mavlinkActive` + GPS fix.
2. **Relative move caps** — enforced in **firmware** (not only JS).
3. **SIM banner** — `build_flags` or compile-time `-D SIMULATION=1` sent in `LINK_STATUS` for UI styling.
4. **Fail-safe** — rely on ArduPilot RTL/failsafe params; GCS shows clear RTL button always visible on map overlay.

---

## SITL vs Pixhawk portability

| Concern | SITL | Pixhawk |
|---------|------|---------|
| Transport | `WiFiClient` :5763 | `Serial2` 115200 |
| Build flag | `SITL_MODE` | omit flag |
| Map / commands | identical | identical |
| SIM banner | on | off |

No branching in JavaScript; only `LINK_STATUS.simulation` boolean.

---

## Testing strategy per phase

- **Unit-level (manual):** serial log + Mission Planner agrees with dashboard action  
- **SITL regression:** arm → takeoff → relative move → RTL → disarm  
- **Latency check:** log command TX → first attitude change &lt; 500 ms typical on LAN  

Document results in phase checklist boxes in [GCS_UPGRADE_ROADMAP.md](./GCS_UPGRADE_ROADMAP.md).
