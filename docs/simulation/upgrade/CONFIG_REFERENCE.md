# Skylink Configuration Reference

All user-tunable values in one place. Change constants here — not scattered in logic.

---

## Build numbers (flash verification)

| What changed | Bump | File(s) | Flash command |
|--------------|------|---------|---------------|
| C++ firmware | `SKYLINK_FIRMWARE_BUILD` | `include/skylink_config.h` | `pio run --target upload` |
| Dashboard / FS | `SKYLINK_FS_BUILD` + `fs` in JSON + `fsBuild` in JS | `skylink_config.h`, `data/skylink_build.json`, `data/gcs_config.js` | `pio run --target uploadfs` |

**Serial boot log:** `Build FW:1 | FS flash:2 (expected 2) OK`  
**Dashboard header:** `FW 1 · FS 2`  
**HTTP check:** `http://<ESP_IP>/health` → `"fw":1,"fs":2,"fs_ok":true`

If `FS flash:0` or `MISMATCH` → run `uploadfs` and bump `fs` in all three FS files.

---

## Firmware — `include/skylink_config.h`

| Constant | Default | Unit | Used by |
|----------|---------|------|---------|
| `SKYLINK_FIRMWARE_BUILD` | 1 | — | Serial log, `/health`, dashboard |
| `SKYLINK_FS_BUILD` | 2 | — | Expected FS (must match `skylink_build.json`) |
| `SKYLINK_PROTOCOL_VERSION` | 1 | — | WebSocket JSON `v` field |
| `SKYLINK_JSON_BUFFER_SIZE` | 768 | bytes | WS serialize buffer |
| `SKYLINK_WS_TELEMETRY_INTERVAL_MS` | 200 | ms | `main.cpp` telemetry to browser (5 Hz) |
| `SKYLINK_WS_LINK_STATUS_INTERVAL_MS` | 1000 | ms | Reserved Phase 1+ |
| `SKYLINK_SITL_TCP_PORT` | 5763 | port | ESP32 ↔ ArduPilot SERIAL2 |
| `SKYLINK_SITL_CONNECT_TIMEOUT_MS` | 3000 | ms | TCP connect timeout |
| `SKYLINK_SITL_RECONNECT_INTERVAL_MS` | 5000 | ms | SITL reconnect backoff |
| `SKYLINK_MAVLINK_GCS_HEARTBEAT_MS` | 1000 | ms | GCS heartbeat to FC |
| `SKYLINK_MAVLINK_STREAM_REQUEST_MS` | 10000 | ms | Re-request telemetry streams |
| `SKYLINK_MAVLINK_TIMEOUT_MS` | 5000 | ms | MAVLink link dead detection |
| `SKYLINK_MOVE_BODY_MAX_M` | 20 | m | Phase 4 relative moves |
| `SKYLINK_MOVE_MIN_AGL_M` | 2 | m | Phase 4 minimum altitude |
| `SKYLINK_GOTO_MAX_RADIUS_M` | 100 | m | Phase 5 click-to-fly |
| `SKYLINK_YAW_MAX_DEG` | 90 | deg | Phase 4 yaw cap |
| `SKYLINK_CMD_RATE_LIMIT_PER_SEC` | 2 | cmd/s | Phase 6 rate limit |
| `SKYLINK_CMD_DEBOUNCE_MS` | 500 | ms | Phase 6 debounce |
| `SKYLINK_LED_MAVLINK_BLINK_MS` | 500 | ms | Phase 1 LED pattern |
| `SKYLINK_LED_ARMED_BLINK_MS` | 200 | ms | Phase 1 optional |

Also see `include/config.h` for WiFi reconnect and serial heartbeat logging.

---

## LittleFS — `data/skylink_build.json`

| Field | Default | Purpose |
|-------|---------|---------|
| `fs` | 2 | FS build on flash (ESP reads at boot) |

---

## Browser — `data/gcs_config.js`

| Key | Default | Purpose |
|-----|---------|---------|
| `fsBuild` | 2 | Display / must match `skylink_build.json` |
| `protocolVersion` | 1 | Must match firmware |
| `wsReconnectInitialMs` | 1500 | WS reconnect start |
| `wsReconnectMaxMs` | 20000 | WS reconnect cap |
| `commsLogMaxEntries` | 40 | Log panel size |
| `defaultTakeoffAltM` | 5 | Takeoff prompt default |
| `takeoffAltMinM` / `MaxM` | 1 / 50 | UI validation only |
| `armModeDelayMs` | 400 | GUIDED before ARM |
| `takeoffArmDelayMs` | 500 | ARM before TAKEOFF |
| `movePresetsM` | [1,3,5,10] | Phase 4 UI buttons |
| `yawPresetsDeg` | [45,90,180] | Phase 4 UI |
| `sitlPortDefault` | 5763 | Display fallback |
| `armHoldMs` | 1500 | Phase 6 hold-to-arm |

---

## After changing config

1. Firmware: `pio run --target upload`
2. UI only: `pio run --target uploadfs`
3. Both changed: `pio run --target uploadfs --target upload`
