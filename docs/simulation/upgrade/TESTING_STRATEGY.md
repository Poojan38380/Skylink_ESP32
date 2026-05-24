# Skylink Testing Strategy

**Goal:** Catch regressions before hardware flight. Testing is a first-class product requirement, not an afterthought.

**Context (May 2026):** MVP verified in SITL (ESP32 ↔ TCP 5763 ↔ ArduPilot). Pixhawk coming later. Stack = PlatformIO + Arduino + AsyncWebServer + MAVLink + browser GCS.

---

## Testing pyramid

```
                    ┌─────────────────────┐
                    │  Manual flight QA   │  rare, scripted checklist
                    │  (SITL + Pixhawk)   │
                    └──────────┬──────────┘
                               │
              ┌────────────────┴────────────────┐
              │   Integration / E2E (automated)  │
              │   SITL + pytest + WS client      │
              └────────────────┬────────────────┘
                               │
        ┌──────────────────────┴──────────────────────┐
        │        Host-native unit tests (Unity)        │
        │  JSON, command table, mavlink encode helpers │
        └──────────────────────┬──────────────────────┘
                               │
        ┌──────────────────────┴──────────────────────┐
        │   Static analysis + CI build + lint          │
        └──────────────────────────────────────────────┘
```

**Rule:** Push tests down the pyramid as far as possible. Do not rely only on “open the dashboard and pray.”

---

## Layer 1 — CI build gate (implement first)

**Purpose:** Every PR / push proves firmware compiles and LittleFS assets exist.

| Check | Command | Fail if |
|-------|---------|---------|
| Firmware build | `pio run -e esp32dev` | compile error |
| FS build | `pio run -e esp32dev -t buildfs` | missing `data/index.html`, `gcs.js`, `gcs.css`, `gcs_config.js` |
| Size budget | optional `pio run -t monitor` size line | flash &gt; 90% |

**GitHub Actions (Windows + Ubuntu):**

```yaml
# .github/workflows/ci.yml (planned)
- checkout
- pip install platformio
- pio run -e esp32dev
- pio run -e esp32dev -t buildfs
- pio test -e native   # when native tests exist
```

**Deliverable:** `.github/workflows/ci.yml` in Phase T1.

---

## Layer 2 — Native unit tests (PlatformIO Unity)

**Purpose:** Test pure logic on your PC in milliseconds — no ESP32 flash, no SITL.

**Tooling:** PlatformIO `test_framework = unity`, `[env:native]` (see [PlatformIO Unity docs](https://docs.platformio.org/en/stable/advanced/unit-testing/frameworks/unity.html)).

### What to test natively (high value)

| Module | Examples |
|--------|----------|
| `isUsableSitlHost()` | rejects `127.0.0.1`, accepts `10.x` |
| Flight mode string → enum | GUIDED → 4 |
| JSON command parsing | mock `deserializeJson`, dispatch table |
| MAVLink packet build | arm, takeoff, set mode — compare byte length + msgid |
| Config constants | `SKYLINK_MOVE_BODY_MAX_M` used in clamp function |
| Geofence math | distance from home &lt; radius |

### What NOT to test on native (yet)

- AsyncWebServer, WiFi, real TCP to SITL
- FreeRTOS mutex timing
- LittleFS serve

### Layout (planned)

```
test/
  native/
    test_sitl_host.cpp
    test_mavlink_commands.cpp
    test_config.cpp
  unity_config.h
```

**Extract** testable functions into `src/skylink_logic.cpp` with `#ifndef ARDUINO` stubs where needed, or compile selected `.cpp` files under `native` with `-D UNIT_TEST`.

**Deliverable:** Phase T2 — ≥15 Unity tests, `pio test -e native` green locally and in CI.

---

## Layer 3 — Host integration tests (Python + pytest)

**Purpose:** Automated SITL + real ESP32 optional + WebSocket client without a browser.

**Tooling:**

- **pytest** orchestrator
- **pymavlink** or raw `websockets` library for WS
- **ArduPilot SITL** subprocess (`sim_vehicle.py -v ArduCopter --no-mavproxy` headless variant if available, or full sim)

### Test harness architecture

```
pytest
  ├── fixtures/
  │     ├── sitl_process    # start/stop SITL, wait for port 5763
  │     ├── esp32_serial    # optional: read serial for log asserts
  │     └── ws_client       # connect to http://ESP_IP/ws
  └── tests/
        ├── test_sitl_ports.py      # 5760/5762/5763 listening
        ├── test_ws_protocol.py     # v=1, HEARTBEAT fields
        ├── test_arm_takeoff.py     # full sequence, read MAVLink STATE
        └── test_relative_move.py   # Phase 4+
```

### Example assertions

- After WS connect, within 15 s: telemetry `sitl_connected == true`
- `SET_FLIGHT_MODE` GUIDED → `COMMAND_ACK` result ACCEPTED
- `TAKEOFF` 5 m → altitude increases &gt; 3 m within 30 s (read via pymavlink on 5762 or WS telemetry)

### SITL in CI

| Option | Pros | Cons |
|--------|------|------|
| GitHub Actions + Ubuntu + SITL | Full auto | Slow (~5 min), flaky if GPU/xterm |
| Scheduled nightly only | Stable PR path | Slower feedback |
| Local `pytest` mandatory pre-push | Fast iteration | Not enforced |

**Recommendation:** PR CI = build + native tests. **Nightly** = SITL integration. **Manual** = demo checklist before thesis submission.

**Deliverable:** Phase T3 — `tests/integration/` + `requirements-test.txt`.

---

## Layer 4 — Web dashboard tests

**Purpose:** UI regressions (broken WS handler, wrong button) caught without flying.

| Approach | Tool | Scope |
|----------|------|-------|
| **Static JS smoke** | Node.js script loads `gcs.js` + `gcs_config.js`, calls pure functions | fmtUptime, config defaults |
| **Browser E2E** | Playwright | open dashboard, mock WS server |
| **Visual** | optional Percy/Chromatic | later |

### Playwright mock WS server

```text
Browser → Playwright → ESP32 OR local static server + mock WS
Mock sends HEARTBEAT JSON → assert map marker moves (Phase 2+)
Click ARM → assert TX command JSON shape
```

**Deliverable:** Phase T4 — Playwright against `data/` served by `python -m http.server` + mock WS.

---

## Layer 5 — Hardware-in-the-loop (HIL)

**When Pixhawk arrives:**

| Stage | Setup |
|-------|--------|
| Bench | Pixhawk USB + ESP32 UART2, props off |
| Tie-in | Same pytest, assert MAVLink on UART instead of SITL TCP |
| Prop-off spin | ARM 2s → DISARM on stand |

**Safety interlock:** pytest must require env `SKYLINK_ALLOW_ARM=1` for motor tests.

---

## Layer 6 — Manual regression scripts (keep forever)

**File:** `docs/simulation/upgrade/MANUAL_REGRESSION.md` (planned)

15-minute script:

1. SITL start → ports OK  
2. Flash ESP32 → dashboard loads `gcs.css` / `gcs.js`  
3. MAVLink OK → arm → takeoff 5 m → forward 5 m (Phase 4) → RTL  
4. Mission Planner agrees  

Sign-off table: date, git SHA, pass/fail.

---

## Test data & fixtures

| Asset | Purpose |
|-------|---------|
| `tests/fixtures/ws_heartbeat.json` | Golden HEARTBEAT payload |
| `tests/fixtures/cmd_takeoff.json` | Golden command |
| `tests/fixtures/mavlink_arm.bin` | Expected packet prefix |

Golden files prevent silent protocol breaks when `SKYLINK_PROTOCOL_VERSION` bumps.

---

## Coverage targets (realistic)

| Area | Target |
|------|--------|
| Native logic | 80% of testable pure functions |
| WebSocket command table | 100% commands have dispatch test |
| Integration | 5 happy-path + 3 failure-path scenarios |
| UI E2E | 3 critical flows (connect, arm blocked, takeoff) |

100% line coverage on ESP32 firmware is **not** a goal (Arduino + async I/O).

---

## Phased implementation plan

| Phase | ID | Deliverable | Est. |
|-------|-----|-------------|------|
| Upgrade 0 | — | Config files, split assets | done |
| Test T1 | CI build | GitHub Actions `pio run` + buildfs | 0.5 day |
| Test T2 | Native Unity | host tests for host IP, mode parse, clamp | 1–2 days |
| Test T3 | pytest SITL | subprocess SITL + WS telemetry test | 2–3 days |
| Test T4 | Playwright | mock WS + button TX | 1–2 days |
| Test T5 | Golden protocol | JSON fixtures in CI | 0.5 day |
| Test T6 | Manual script | MANUAL_REGRESSION.md | 0.5 day |
| Test T7 | Pixhawk HIL | extend pytest for UART | when hardware arrives |

**Run order:** T1 → T2 → T6 (manual doc) → T3 → T4 → T5.

---

## Tooling references (2025–2026)

- PlatformIO Unity: https://docs.platformio.org/en/stable/advanced/unit-testing/frameworks/unity.html  
- ESP-IDF unit test patterns: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/unit-tests.html  
- pytest-embedded (Espressif): for advanced ESP32 on-target tests later  
- ArduPilot autotest: reference for SITL parameter patterns (optional reuse)

---

## Definition of done — “Skylink is tested”

- [ ] CI green on every push (build + native)  
- [ ] Nightly or manual pytest proves SITL WS arm/takeoff path  
- [ ] Config constants documented in CONFIG_REFERENCE.md  
- [ ] Manual regression completed before each demo / thesis milestone  
- [ ] Known flaky tests documented with retry policy  

---

## Agent instructions

When adding a feature from [GCS_UPGRADE_ROADMAP.md](./GCS_UPGRADE_ROADMAP.md):

1. Add native test if pure logic exists.  
2. Add golden JSON if protocol changes.  
3. Extend pytest if flight behavior changes.  
4. Update MANUAL_REGRESSION.md checklist.  

**Never merge flight-control changes without at least one automated test or explicit manual script update.**
