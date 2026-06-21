# Skylink Indoor Motor Spin Test — No Props

**Purpose:** Verify all 4 motors spin correctly and respond to throttle when the drone attempts takeoff, without risk of flight. Run this BEFORE any outdoor test.

---

## Safety Rules (Read First)

> ⚠️ Even without props, spinning motors can suck in wires/fingers and generate heat.
> 1. Remove ALL propellers before starting.
> 2. Have one person hold the drone firmly flat on a table or floor — do NOT let go.
> 3. Keep loose wires, clothing, and fingers well away from motors.
> 4. Have a second person at the keyboard/dashboard ready to cut power immediately.
> 5. Keep the test under 5 seconds of motor spin.
> 6. If any motor spins unevenly, rattles, or smells hot — kill power immediately.

---

## What You're Testing

| Check | Pass criteria |
|---|---|
| All 4 motors arm | Motors give a beep sequence when armed |
| All 4 motors spin on throttle | All visibly spin after takeoff command |
| Motor direction | Each motor spins in the correct direction (CW/CCW per frame layout) |
| ESC calibration | All motors spin at roughly the same speed at same throttle |
| No vibration from imbalance | Drone doesn't shake excessively |
| ArduPilot accepts the takeoff command | Dashboard shows no FAILED/DENIED ACK |

---

## Setup

1. **Remove all propellers.** Double-check. Triple-check.
2. Power FC + ESCs + motors via normal LiPo (NOT just USB).
3. Connect ESP32 to WiFi, open dashboard.
4. Make sure Serial monitor (`pio device monitor`) is running in another terminal.
5. Place drone on a hard flat surface. Have someone firmly hold it down from the top frame — not touching motor bells.
6. In Mission Planner (or dashboard Status tab), confirm:
   - GPS 3D fix acquired (or accept that it may not arm without GPS if ARMING_CHECK is not 0)
   - ARMING_CHECK parameter = 0 (set via Mission Planner → Config → Full Parameter List → search ARMING_CHECK → set to 0)

---

## Test Procedure

### Step 1 — Manual arm from dashboard
1. Open the **Fly** tab.
2. Click **"Set Mode: GUIDED"** manually first.
3. Wait for Mode chip to show GUIDED in the live strip.
4. Click **"ARM"**.
5. **Expected:** Motors give arming beep sequence. All 4 ESCs armed.
6. If armed: motors should be at idle (just barely spinning or stationary — depends on ESC config).

### Step 2 — Autonomous takeoff (low altitude)
1. Ensure someone is holding the drone firmly.
2. Click **Takeoff** in the Fly tab, enter **1 m**.
3. **Expected serial log sequence:**
   ```
   [INFO] Setting flight mode: 4
   [INFO] Sending command: ARM Drone
   [INFO] Sending command: TAKEOFF to 1.00m
   ```
4. **Expected dashboard log:**
   ```
   SYS  [TAKEOFF 1/6] Sending GUIDED mode…
   ACK  Command 176: ACCEPTED
   SYS  [TAKEOFF 2/6] GUIDED ACK accepted — waiting for GUIDED state…
   SYS  [TAKEOFF 3/6] GUIDED confirmed — sending ARM…
   ACK  Command 400: ACCEPTED
   SYS  [TAKEOFF 4/6] ARM ACK accepted — waiting for ARMED state…
   SYS  [TAKEOFF 5/6] Armed in GUIDED confirmed — sending TAKEOFF…
   ACK  Command 22: ACCEPTED
   SYS  [TAKEOFF 6/6] TAKEOFF ACK accepted for 1 m
   ```
5. **Expected motor behaviour:** All 4 motors spin up significantly and sustain — the FC is trying to climb to 1m.
6. Hold the drone for 3–5 seconds to observe.
7. Click **LAND** or **DISARM** to stop.

---

## Diagnosing Motor Issues

### Only some motors spin
- ESC not calibrated for that motor
- Motor wire loose or broken
- ESC output channel wrong in ArduPilot Motor Test (Config → Optional Hardware → Motor Test)

### Motors spin but unevenly
- ESC calibration drift — recalibrate all ESCs together
- Throttle curve issue

### Drone tries to roll/flip immediately
- Wrong motor direction on one or more motors (swap any 2 wires on the motor to reverse)
- Wrong prop direction assigned in ArduPilot (motor numbering mismatch)

### No motors spin at all after ARM
- ARMING_CHECK blocking it (look at STATUSTEXT in dashboard Status tab)
- Safety switch not pressed (if your FC has one)
- ESC not powered from LiPo

### ACK Command 22 FAILED/DENIED
- FC is not in GUIDED mode at the time TAKEOFF is sent
- GPS fix type < 3 AND ARMING_CHECK != 0

---

## Motor Test via Mission Planner (Alternative)
If the above fails, use Mission Planner's built-in motor test:
1. Connect FC to Mission Planner via USB.
2. Go to **Config → Optional Hardware → Motor Test**.
3. Set throttle % to 5% and duration to 2 seconds.
4. Click Test Motor A, B, C, D individually.
5. This bypasses arming entirely — useful for isolating which motor/ESC has issues.

---

## What to Log
After the test, note in your test log:
- Did all 4 motors arm? Y/N
- Did all 4 spin on takeoff command? Y/N
- Were any motors noticeably slower/faster? Which?
- Any STATUSTEXT messages from FC during the test?
- Any ACK FAILED or DENIED in dashboard log?
