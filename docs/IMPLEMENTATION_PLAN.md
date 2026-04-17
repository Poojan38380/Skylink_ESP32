# Skylink ESP32 — Implementation Plan
## From Testing Code → Professor-Ready UAV Remote Control Demo

> **Target**: A polished, working ESP32 demo proving internet-based UAV control concepts.
> **Stack**: Arduino Core (PlatformIO) · ESPAsyncWebServer · AsyncTCP · LittleFS · ArduinoJson

---

## Current State Assessment

| Module | File | Current State | Problem |
|---|---|---|---|
| WiFi Manager | `wifi_manager.cpp` | Single hardcoded SSID/password | Buggy reconnect, no fallback list |
| Config Manager | `config_manager.cpp` | Preferences (flash KV store) | No JSON file support |
| Web Server | `web_server.cpp` | Basic `WebServer.h` (HTTP only) | No WebSocket support |
| Main Loop | `main.cpp` | Heartbeat blinks LED only | Not WebSocket-driven |
| Config | `config.cpp` | Hardcoded personal SSID | Not shareable / portable |
| Dashboard HTML | (inline in web_server.cpp) | Plain HTML, looks like a test app | Must look like a GCS (Ground Control Station) |

---

## Architecture After Implementation

```
wifi_networks.json  (LittleFS flash filesystem)
         │
         ▼
[Phase 1] WiFiManager  ─── scans → matches → connects
         │
         ▼
[Phase 2] ESPAsyncWebServer ──► GET /         → GCS Dashboard (index.html)
                            ──► GET /health   → { "status": "ok" }
                            ──► WebSocket /ws → bidirectional JSON messages
         │
         ▼
[Phase 3] WebSocket Handler
    Browser → ESP32:  { type:"command", command:"LED_SET", value:true }
    ESP32 → Browser:  { type:"event", event:"LED_STATE", value:true, ... }
    ESP32 → Browser:  { type:"event", event:"HEARTBEAT", altitude:12.4, battery:87 }

[Phase 4] Dashboard UI
    - Connection indicator (live WebSocket status)
    - LED control panel (ON / OFF / TOGGLE)
    - Simulated telemetry (altitude, battery, GPS, speed)
    - Serial-style message log
```

---

## Library Dependencies to Add

Add these to `platformio.ini`:

```ini
lib_deps =
    ESP Async WebServer
    AsyncTCP
    bblanchon/ArduinoJson @ ^7.0.0
```

LittleFS is built into the ESP32 Arduino core — no extra dependency needed.

---

## Phase 1: WiFi Overhaul — Saved Network JSON + LittleFS

**Goal**: Replace fragile single-credential WiFi with a scan → match → connect flow 
driven by a JSON file on the ESP32's flash filesystem.

### Files Created / Modified

| File | Action |
|---|---|
| `data/wifi_networks.json` | **NEW** — the saved networks config file |
| `include/wifi_manager.h` | **MODIFY** — add LittleFS + JsonDocument support |
| `src/wifi_manager.cpp` | **REWRITE** — implement scan-and-match logic |
| `include/config.h` | **MODIFY** — remove hardcoded SSID/PASSWORD externs |
| `src/config.cpp` | **MODIFY** — remove hardcoded credentials |
| `platformio.ini` | **MODIFY** — add LittleFS board_build flag + lib_deps |

### `data/wifi_networks.json` Contents

```json
{
  "networks": [
    {
      "ssid": "Pixel 6",
      "password": "123456785",
      "priority": 1
    },
    {
      "ssid": "Poojan's AsusVivobook",
      "password": "qwertyuio",
      "priority": 2
    }
  ]
}
```

> **Note**: Priority 1 = first tried. Add more networks freely to this file.
> Upload the `data/` folder using: `pio run --target uploadfs`

### New WiFiManager Logic (scan-and-match)

```
boot
 └─► LittleFS.begin()
 └─► parse wifi_networks.json → load network list (sorted by priority)
 └─► WiFi.scanNetworks()
 └─► for each saved network (in priority order):
       is its SSID in the scan results?
         YES → WiFi.begin(ssid, password) → wait up to 15s
               connected? → done ✓
               failed?    → try next network
         NO  → skip
 └─► all failed → log error, retry after reconnectInterval
```

### ✅ Phase 1 Verification Checklist

After flashing Phase 1 firmware + filesystem:

1. Open Serial Monitor (`pio device monitor`) — baud **115200**
2. You must see:
   - `[INFO] LittleFS mounted`
   - `[INFO] Loaded N saved networks from wifi_networks.json`
   - `[INFO] Scanning for available networks...`
   - `[INFO] Found target: Pixel 6` (or whichever is in range)
   - `[INFO] WiFi connected! IP: 192.168.x.x`
3. **Failure test**: Turn off hotspot → after `WIFI_RECONNECT_INTERVAL` (10s) you must see `[WARNING] WiFi disconnected, attempting to reconnect...`
4. **File not found test**: If `data/` was not uploaded, you must see `[ERROR] wifi_networks.json not found! Using fallback.` and the device should **not** crash.

---

## Phase 2: ESPAsyncWebServer + WebSocket Endpoint

**Goal**: Replace the blocking `WebServer.h` with `ESPAsyncWebServer`, add a `/ws` 
WebSocket endpoint, and implement JSON message parsing.

### Files Created / Modified

| File | Action |
|---|---|
| `include/web_server.h` | **REWRITE** — use `AsyncWebServer` + `AsyncWebSocket` |
| `src/web_server.cpp` | **REWRITE** — async handlers + WebSocket message dispatcher |
| `include/led_controller.h` | **NEW** — LED state encapsulated as its own module |
| `src/led_controller.cpp` | **NEW** — `setLED(bool)`, `toggleLED()`, `getLEDState()` |
| `src/main.cpp` | **MODIFY** — remove old heartbeat LED blink, add ws broadcast call |

### Message Schema (must be implemented exactly — reused in Phase 3 cloud)

**Browser → ESP32 (commands)**
```json
{ "type": "command", "command": "LED_SET",    "value": true }
{ "type": "command", "command": "LED_TOGGLE"               }
{ "type": "command", "command": "PING"                     }
```

**ESP32 → Browser (events)**
```json
{ "type": "event", "event": "LED_STATE",  "value": true,  "timestamp": 12345 }
{ "type": "event", "event": "PONG",                        "timestamp": 12345 }
{ "type": "event", "event": "HEARTBEAT",  "uptime": 60,   "timestamp": 12345,
  "altitude": 12.4, "battery": 87, "lat": 19.0760, "lng": 72.8777, "speed": 0.0 }
{ "type": "event", "event": "ERROR",      "message": "Unknown command" }
```

> **IMPORTANT**: Keep this schema frozen. Phase 3 (cloud) will reuse it verbatim.

### ✅ Phase 2 Verification Checklist

1. Serial monitor must show `[INFO] WebSocket server started at /ws`
2. Open browser → `http://<ESP32_IP>/` → page must load (even if plain HTML)
3. Open browser DevTools → Console → no JS errors
4. Open DevTools → Network → WS tab → you must see the WebSocket handshake (101 Switching Protocols)
5. After 3 seconds, the console must show a `HEARTBEAT` event arriving
6. Send a `PING` from DevTools Console:
   ```js
   ws.send(JSON.stringify({type:"command", command:"PING"}))
   ```
   You must receive `{ type:"event", event:"PONG", ... }` back
7. Send `LED_SET true` — the ESP32's built-in LED must physically turn ON
8. Send malformed JSON (e.g. `"not json"`) — must NOT crash ESP32; must receive `ERROR` event

---

## Phase 3: GCS Dashboard UI (Ground Control Station)

**Goal**: Replace the plain HTML with a polished, dark-themed GCS dashboard that 
impresses the professor. All served from the ESP32 itself.

### Files Created / Modified

| File | Action |
|---|---|
| `data/index.html` | **NEW** — full GCS dashboard (served via LittleFS) |
| `src/web_server.cpp` | **MODIFY** — serve `index.html` from LittleFS instead of inline HTML |

### Dashboard UI Sections

```
┌─────────────────────────────────────────────────────────┐
│  🛰 SKYLINK UAV GROUND CONTROL STATION          [● LIVE] │
├──────────────────┬──────────────────────────────────────┤
│  CONNECTION      │  TELEMETRY                           │
│  ● Connected     │  Altitude:  12.4 m                   │
│  IP: 192.168.x.x │  Battery:   87%  ████████░░          │
│  Signal: -62 dBm │  Speed:     0.0 m/s                  │
│  Uptime: 00:05:12│  GPS: 19.0760°N  72.8777°E           │
├──────────────────┴──────────────────────────────────────┤
│  LED CONTROL (Demo Actuator)                            │
│  [  LED ON  ]  [  LED OFF  ]  [  TOGGLE  ]             │
│  Status: ● ON                                           │
├─────────────────────────────────────────────────────────┤
│  MESSAGE LOG                                            │
│  [00:01:23] HEARTBEAT — alt:12.4m bat:87%              │
│  [00:01:20] LED_STATE — ON                              │
│  [00:01:18] PING → PONG (12ms)                          │
└─────────────────────────────────────────────────────────┘
```

**Design requirements**:
- Dark background (`#0d1117`), accent color electric blue (`#00d4ff`)
- Monospace font (JetBrains Mono or similar from Google Fonts)
- Animated "heartbeat pulse" on the status dot when live
- Battery bar changes color: green > 50%, orange > 20%, red ≤ 20%
- Simulated altitude value gently oscillates (±0.5 m noise) to look "live"
- Simulated GPS coordinates slightly drift each heartbeat
- Auto-reconnect with countdown shown in UI

### ✅ Phase 3 Verification Checklist

1. `http://<ESP32_IP>/` must load the GCS dashboard (not a 404 or plain page)
2. Connection dot must be green and animated when WebSocket is live
3. Telemetry values must update every ~3 seconds with slight variation
4. LED ON/OFF/TOGGLE buttons must physically control the ESP32 LED and update UI
5. Message log must show the last 10 messages, scrollable
6. **Disconnect test**: Unplug ESP32 from USB briefly → dashboard must show "Disconnected" in red → reconnect → dashboard must recover automatically without page refresh
7. Test on mobile browser (phone on same WiFi) — layout must be usable

---

## Phase 4: Polish + Professor Demo Prep

**Goal**: Harden reliability, clean up serial logs, and make the demo bulletproof.

### Tasks

| Task | Details |
|---|---|
| WiFi reconnect on web server | If WiFi drops and reconnects, `AsyncWebServer` must survive (`server.begin()` is persistent — verify) |
| OTA still works | `ArduinoOTA.handle()` must coexist with `AsyncWebServer` (they are compatible — verify) |
| NTP time in heartbeat | Use real NTP time in heartbeat `timestamp` field instead of `millis()` |
| Serial log cleanup | Change logger level to `LOG_INFO` in release (suppress DEBUG noise) |
| `GET /health` endpoint | Returns `{"status":"ok","uptime":1234,"ip":"192.168.x.x"}` |
| Memory check | `ESP.getFreeHeap()` must remain above 60 KB during normal operation |

### ✅ Phase 4 Verification Checklist

1. Run for **10 minutes continuously** → no crash, no memory leak in serial log
2. OTA upload still works: `pio run --target upload --upload-port <ESP32_IP>`
3. `GET http://<ESP32_IP>/health` returns valid JSON
4. Serial shows real NTP timestamps in HEARTBEAT (e.g., `2026-04-17 15:30:00`) not just millis
5. `ESP.getFreeHeap()` logged in heartbeat must stay stable (not steadily declining)
6. **Full cold-boot demo**: Power on ESP32 → within 20 seconds dashboard is live in browser

---

## File Map: Before vs After

```
Before (test code)          After (professor demo)
─────────────────────────── ─────────────────────────────────────
include/config.h            include/config.h          (trimmed)
include/wifi_manager.h      include/wifi_manager.h    (LittleFS + JSON)
include/web_server.h        include/web_server.h      (AsyncWebServer)
include/config_manager.h    include/config_manager.h  (unchanged)
include/logger.h            include/logger.h          (unchanged)
include/ota_updater.h       include/ota_updater.h     (unchanged)
include/time_sync.h         include/time_sync.h       (unchanged)
                            include/led_controller.h  [NEW]

src/config.cpp              src/config.cpp            (no hardcoded creds)
src/wifi_manager.cpp        src/wifi_manager.cpp      (full rewrite)
src/web_server.cpp          src/web_server.cpp        (full rewrite)
src/main.cpp                src/main.cpp              (updated)
src/config_manager.cpp      src/config_manager.cpp    (unchanged)
src/logger.cpp              src/logger.cpp            (unchanged)
src/ota_updater.cpp         src/ota_updater.cpp       (unchanged)
src/time_sync.cpp           src/time_sync.cpp         (unchanged)
                            src/led_controller.cpp    [NEW]

                            data/wifi_networks.json   [NEW]
                            data/index.html           [NEW]

platformio.ini              platformio.ini            (lib_deps + LittleFS)
```

---

## How to Build and Flash (Each Phase)

```bash
# 1. Flash firmware
pio run --target upload

# 2. Upload filesystem (wifi_networks.json + index.html)
pio run --target uploadfs

# 3. Monitor serial output
pio device monitor --baud 115200

# 4. OTA flash (after Phase 1 is on the device and WiFi is connected)
pio run --target upload --upload-port <ESP32_IP>
```

---

## Phase Execution Order

```
Phase 1 ──► Verify WiFi works with JSON ──► Phase 2
Phase 2 ──► Verify WebSocket works      ──► Phase 3
Phase 3 ──► Verify GCS UI loads + live  ──► Phase 4
Phase 4 ──► 10-min stability test       ──► Demo ready ✓
```

Do NOT proceed to the next phase until every checkbox in the current phase is green.
