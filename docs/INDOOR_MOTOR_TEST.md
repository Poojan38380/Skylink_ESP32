# Skylink Indoor Motor Test — No Props (Bench Tab)

**Purpose:** Verify MAVLink link, arm/disarm, and per-motor spin on the bench **without propellers**. This is **testing only** — takeoff is **not** part of bench testing.

Use the dashboard **Bench** tab (hardware builds only). Takeoff remains on the **Fly** tab for outdoor flight with GPS.

---

## Safety rules

> Even without props, spinning motors can pull in wires or fingers and generate heat.

1. Remove **all** propellers before starting.
2. Hold the drone firmly on a table — do not let go during ARM tests.
3. Keep fingers, clothing, and loose wires away from motor bells.
4. Have a second person ready to disarm or cut power.
5. Limit each motor test to **2–3 seconds** at **≤10% throttle** (dashboard caps at 15% / 3 s).
6. Restore **`ARMING_CHECK=1`** before any outdoor flight.

---

## Pixhawk bench setup (Mission Planner)

Indoor bench tests do **not** require GPS in Skylink, but Pixhawk may still block arm until checks are relaxed:

| Parameter | Bench value | Notes |
|---|---|---|
| `ARMING_CHECK` | `0` (or exclude GPS/compass) | Restore before outdoor flight |
| `SERIAL2_PROTOCOL` | `2` (MAVLink2) | TELEM2 to ESP32 |
| `SERIAL2_BAUD` | `115200` | Match ESP32 UART2 |

Wiring: ESP32 RX2 (GPIO16) ← Pixhawk TELEM2 TX; ESP32 TX2 (GPIO17) → Pixhawk TELEM2 RX; common ground.

---

## What the Bench tab tests

### Group A — Link & comms (no GPS gate)

- WebSocket connected
- Bench mode activated (`BENCH_MODE_ON`)
- PING burst → RTT p50/p95
- MAVLink heartbeat fresh
- Telemetry fields present
- `SET_FLIGHT_MODE`: STABILIZE → GUIDED → LAND (ACK each)

### Group B — Props-off mechanical (no GPS gate)

After ticking all four confirmations:

1. `ARM_DRONE` → hold drone
2. `DISARM_DRONE`
3. `MOTOR_TEST` motors A–D (MAV_CMD_DO_MOTOR_TEST @ 10%, 2 s)

**Excluded from bench:** TAKEOFF, MOVE_BODY, GOTO, RTL, LOITER, RC_OVERRIDE.

---

## Operator procedure

1. Remove props. Power Pixhawk + ESP32 (LiPo for ESCs if testing motors).
2. Set `ARMING_CHECK=0` in Mission Planner if indoor arm without GPS is needed.
3. Open dashboard → **Bench** tab (activates bench mode automatically).
4. **Run Group A** → review metrics and checklist table → **Export CSV**.
5. Tick all props-off confirmations.
6. **Run Group B** (or use manual Arm / Disarm / Motor buttons).
7. Export CSV for latency evidence (paper / lab log).
8. **Disarm**, leave bench tab (deactivates bench mode), restore `ARMING_CHECK=1`.

---

## Pass criteria

| Check | Pass |
|---|---|
| Group A completes without FAIL rows | All link/comms steps PASS |
| ARM ACK accepted | `Command 400: ACCEPTED` in log |
| DISARM ACK accepted | Vehicle disarmed in telemetry |
| Each MOTOR_TEST ACK | `Command 209: ACCEPTED`; motor spins ~2 s |
| No unexpected STATUSTEXT errors | No persistent prearm failures |

---

## Troubleshooting

### ARM denied (STATUSTEXT)

- Pixhawk pre-arm still blocking (GPS, compass, throttle) — adjust `ARMING_CHECK` or fix hardware.
- Safety switch not pressed (if fitted).

### MOTOR_TEST FAILED/DENIED

- FC firmware may require disarmed state for motor test.
- Motor channel mapping wrong — verify in Mission Planner Motor Test page.

### ACK timeout in UI but serial shows ACK

- Reflash firmware FS build ≥23 (ACK prioritization + post-command flush).
- Reduce STATUSTEXT flood from FC if possible.

### Mission Planner motor test (fallback)

Config → Optional Hardware → Motor Test — 5% throttle, 2 s, test A–D individually. Bypasses Skylink; useful to isolate ESC wiring.

---

## What to log

Export CSV from Bench tab and note:

- Date, FW/FS build, Wi‑Fi RSSI, min heap
- PING p50/p95, per-step ACK ms
- ARM / DISARM / motor results
- Any FC STATUSTEXT warnings

**Takeoff is never part of this procedure.**
