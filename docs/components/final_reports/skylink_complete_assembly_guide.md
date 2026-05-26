# Skylink Drone — Complete Assembly & Operations Guide

> [!IMPORTANT]
> This is the **definitive, single-source-of-truth** guide for building, wiring, programming, and flying your Skylink internet-controlled F450 quadcopter. Every step is derived from the actual Skylink firmware codebase, your ordered components, and your existing inventory.
>
> **Author:** Skylink Project — May 2026  
> **Firmware Baseline:** Skylink FW 12 / FS 15 | ArduCopter 4.x  
> **Control Mode:** Internet-only (no RC transmitter). GCS via 4G cellular + WiFi.

---

## Table of Contents

1. [Complete Parts Inventory](#1-complete-parts-inventory)
2. [Additional Parts You Still Need to Buy](#2-additional-parts-you-still-need-to-buy)
3. [Tools Required](#3-tools-required)
4. [System Architecture](#4-system-architecture)
5. [Phase 1 — Frame Assembly](#phase-1--frame-assembly-45-minutes)
6. [Phase 2 — Motor & ESC Wiring](#phase-2--motor--esc-wiring-2-hours)
7. [Phase 3 — Pixhawk Mounting & Core Wiring](#phase-3--pixhawk-mounting--core-wiring-1-hour)
8. [Phase 4 — ESP32 Companion Computer Integration](#phase-4--esp32-companion-computer-integration-1-hour)
9. [Phase 5 — 4G Modem Integration](#phase-5--4g-modem-integration-30-minutes)
10. [Phase 6 — Power Distribution & Battery](#phase-6--power-distribution--battery-1-hour)
11. [Phase 7 — Firmware & Software Setup](#phase-7--firmware--software-setup-2-hours)
12. [Phase 8 — Bench Testing (NO PROPS)](#phase-8--bench-testing-no-props)
13. [Phase 9 — First Flight Operations](#phase-9--first-flight-operations)
14. [Critical ArduPilot Parameters](#10-critical-ardupilot-parameters)
15. [Troubleshooting Reference](#11-troubleshooting-reference)
16. [Pre-Flight Checklist](#12-pre-flight-checklist)

---

## 1. Complete Parts Inventory

### ✅ What You Are Ordering (5 Items)

| # | Component | Vendor | Price (₹) | Status |
|---|-----------|--------|-----------|--------|
| 1 | **Pixhawk 2.4.8 Combo Kit** (includes GPS Neo-M8N, Power Module, Safety Switch, Buzzer, Shock Mount, SD Card, Cables) | Robokits (RKI-7227) | ~₹6,500 | 🛒 Ordering |
| 2 | **F450 Quadcopter Frame Kit** (includes 4× A2212 KV1000 Motors, 4× 30A ESCs, 2× pairs 1045 Props) | Robocraze | ~₹3,800 | 🛒 Ordering |
| 3 | **7Semi EC200U LTE 4G GNSS Mini Industrial Modem** (USB-C, 5–26V Range) | Robodo.in | ₹1,769 | 🛒 Ordering |
| 4 | **XT60 Male Connector Pigtail** (18AWG, 10cm) | Robocraze | ~₹80 | 🛒 Ordering |
| 5 | **4mm Heat Shrink Tube** (Black, 1 meter) | Robocraze | ₹13 | 🛒 Ordering |

### ✅ What You Already Own

| # | Component | Role in Skylink |
|---|-----------|----------------|
| 1 | **ESP32 DevKit V1 (30-pin, WROOM-32)** | Skylink Companion Computer — runs web server, MAVLink bridge, GCS dashboard |
| 2 | **XL4015 Buck Converter Module (5A)** | Isolated 5V power supply for ESP32 from LiPo battery |
| 3 | **Solderless Breadboard (830-point)** | Bench prototyping of ESP32 + Pixhawk wiring before permanent install |
| 4 | **DuPont Male-to-Male Jumper Wires** | Signal wiring (UART, power) during prototyping |
| 5 | **Perfboards (Prototyping Boards)** | Final permanent wiring harness to replace breadboard |
| 6 | Arduino Uno R3 Clone | Not used in Skylink (backup microcontroller) |
| 7 | Power Bank Modules | Not used in Skylink |
| 8 | RFID Fobs & Cards | Not used in Skylink |

---

## 2. Additional Parts You Still Need to Buy

> [!CAUTION]
> **You CANNOT fly without these items.** They are not included in any of the kits you are ordering.

### 🔴 CRITICAL — Cannot Fly Without These

| # | Component | Why It's Essential | Estimated Price (₹) | Where to Buy |
|---|-----------|-------------------|---------------------|--------------|
| 1 | **3S LiPo Battery (11.1V, 2200–3300mAh, 25C–35C, XT60 connector)** | The ONLY power source for the entire drone — motors, ESCs, Pixhawk, ESP32, modem. Without this, nothing powers on. | ₹1,200 – ₹2,200 | Robokits, Robocraze, RC hobby shops |
| 2 | **LiPo Balance Charger** (IMAX B6, B6AC, or SkyRC e3/e4) | LiPo batteries **WILL catch fire** if charged incorrectly. A balance charger monitors each cell independently. You cannot charge a LiPo from a normal phone charger. | ₹1,200 – ₹3,500 | Robokits, Robocraze, Amazon India |
| 3 | **Active 4G SIM Card** (Nano-SIM with data plan — Jio/Airtel) | The 7Semi EC200U modem needs a SIM card with an active data plan to connect to the internet. Without this, no internet = no drone control. | ₹100 + ₹149/month plan | Jio/Airtel store |

### 🟡 STRONGLY RECOMMENDED — Safety & Reliability

| # | Component | Why You Need It | Estimated Price (₹) | Where to Buy |
|---|-----------|----------------|---------------------|--------------|
| 4 | **Soldering Iron (60W+) + Solder Wire + Flux** | The ESC power wires MUST be soldered to the F450 PDB (center plate). DuPont wires cannot carry 30A motor currents. This is not optional. | ₹400 – ₹1,200 | Local electronics shop |
| 5 | **LiPo Safe Bag (Fireproof)** | For storing and charging the LiPo battery safely. LiPo thermal runaway is a real risk. | ₹200 – ₹500 | Robocraze, Amazon India |
| 6 | **Zip Ties (Nylon, assorted sizes)** | Secure ESCs to arms, bundle wires, mount GPS mast. Used everywhere in drone assembly. | ₹50 – ₹100 | Hardware store |
| 7 | **Double-Sided Foam Tape (3M VHB or similar)** | Mount Pixhawk (vibration damping), mount ESP32, mount modem to frame. | ₹100 – ₹200 | Stationery / hardware shop |
| 8 | **Multimeter** (any basic digital multimeter) | Verify XL4015 output is 5.0V, check for shorts, test continuity on solder joints. Essential for safe power setup. | ₹300 – ₹600 | Local electronics shop |
| 9 | **Micro-USB Data Cable** (for ESP32 programming) | NOT a charge-only cable. Must support data transfer for PlatformIO firmware upload. You may already have one. | ₹50 – ₹150 | Electronics shop |
| 10 | **Female-to-Female DuPont Wires** (or Male-to-Female) | The Pixhawk TELEM2 port uses a 6-pin DF13 connector. You need F-F wires or a breakout cable to connect to the ESP32's male header pins. | ₹50 – ₹100 | Electronics shop |

### 🟢 OPTIONAL BUT NICE TO HAVE

| # | Component | Why | Price (₹) |
|---|-----------|-----|-----------|
| 11 | **GPS Antenna for 7Semi Modem** (u.FL/IPEX Active GPS Antenna) | Enables the modem's built-in GNSS for dual-redundant position tracking alongside the Pixhawk GPS. | ₹200 – ₹400 |
| 12 | **FlySky FS-i6X RC Transmitter + FS-iA6B Receiver** | Emergency manual override. Lets you take physical control if the internet link drops. Highly recommended for first flights. | ₹3,500 – ₹4,500 |
| 13 | **Spare 1045 Propellers (2 pairs)** | Props break on hard landings. Having spares saves a trip to the shop. | ₹100 – ₹200 |
| 14 | **Landing Gear / Tall Legs** | Protects the battery and electronics mounted underneath from ground impact. | ₹200 – ₹400 |
| 15 | **Thread-locker (Loctite 243 Blue)** | Prevents motor mount screws and frame bolts from vibrating loose during flight. | ₹150 |

### 💰 Estimated Additional Cost Summary

| Category | Min (₹) | Max (₹) |
|----------|---------|---------|
| 🔴 Critical (Battery + Charger + SIM) | ₹2,500 | ₹5,800 |
| 🟡 Strongly Recommended (Solder + Safety) | ₹1,150 | ₹2,850 |
| 🟢 Optional | ₹4,150 | ₹5,650 |
| **Total Additional** | **₹3,650** | **₹14,300** |

---

## 3. Tools Required

| Tool | Purpose | You Have? |
|------|---------|-----------|
| Soldering iron (60W+) | ESC-to-PDB, power wires | ❌ Need to buy |
| Solder wire (60/40 rosin core) | Solder joints | ❌ Need to buy |
| Multimeter | Voltage verification | ❌ Need to buy |
| Hex key set (M2, M2.5, M3) | Frame bolts, motor screws | ✅ Usually included with F450 kit |
| Screwdriver set (Phillips #1, #2) | Various assembly | ✅ Common household |
| Wire stripper / cutter | Preparing wires | ✅ Or use scissors/knife carefully |
| Small flathead screwdriver | XL4015 potentiometer adjustment | ✅ Common household |
| Computer with VS Code + PlatformIO | Firmware compilation and upload | ✅ Your laptop |
| Heat gun or lighter | Shrinking heat-shrink tubes | ✅ Lighter works in a pinch |

---

## 4. System Architecture

### 4.1 High-Level Data Flow

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                           SKYLINK SYSTEM ARCHITECTURE                           │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                 │
│   ┌──────────────┐     4G LTE      ┌──────────────┐    WiFi      ┌──────────┐  │
│   │  Your Phone  │◄───────────────►│  7Semi EC200U │◄───────────►│  ESP32   │  │
│   │  or Laptop   │    Internet      │  4G Modem     │   Hotspot   │ Skylink  │  │
│   │  (Browser)   │                  │  (SIM Card)   │    or LAN   │ Bridge   │  │
│   └──────────────┘                  └──────────────┘             └────┬─────┘  │
│         │                                                              │        │
│    GCS Dashboard                                                  MAVLink      │
│    (gcs.js / map)                                                  UART2       │
│                                                               GPIO16/17       │
│                                                              115200 baud      │
│                                                                    │          │
│                                                              ┌─────▼──────┐   │
│                     ┌──────────┐   I2C/UART    ┌─────────┐   │  Pixhawk   │   │
│                     │ Neo-M8N  │◄─────────────►│ Compass │   │   2.4.8    │   │
│                     │   GPS    │               │ (HMC)   │   │ ArduCopter │   │
│                     └──────────┘               └─────────┘   └─────┬──────┘   │
│                                                                    │          │
│                                                              MAIN OUT 1-4     │
│                                                                    │          │
│                                                     ┌──────────────┼──────┐   │
│                                                     ▼    ▼    ▼    ▼      │   │
│                                                    ESC  ESC  ESC  ESC     │   │
│                                                     │    │    │    │      │   │
│                                                     M1   M2   M3   M4    │   │
│                                                   (CW) (CCW)(CW) (CCW)   │   │
│                                                                           │   │
└───────────────────────────────────────────────────────────────────────────────┘
```

### 4.2 Power Distribution Map

```
                        ┌───────────────────────────┐
                        │  3S LiPo Battery (11.1V)  │
                        │  2200–3300mAh, 25-35C      │
                        └─────────────┬─────────────┘
                                      │
                                  XT60 Plug
                                      │
                        ┌─────────────▼─────────────┐
                        │  Pixhawk Power Module     │
                        │  (Current + Voltage Sense) │
                        └──────┬──────────────┬─────┘
                               │              │
                    5.3V Regulated      Raw Battery
                    (6-pin cable)      Pass-Through
                               │              │
                        ┌──────▼──────┐  ┌────▼──────────────────┐
                        │  Pixhawk    │  │  F450 Center Plate    │
                        │  POWER Port │  │  PDB (Power Dist.)    │
                        └─────────────┘  └──┬───┬───┬───┬───┬───┘
                                            │   │   │   │   │
                                           ESC ESC ESC ESC  │
                                           1   2   3   4    │
                                           │   │   │   │    │
                                           M1  M2  M3  M4   │
                                                             │
                                          ┌──────────────────┤
                                          │                  │
                                  ┌───────▼────────┐  ┌─────▼──────────┐
                                  │  XL4015 Buck   │  │  7Semi EC200U  │
                                  │  Converter     │  │  4G Modem      │
                                  │  IN: 11.1V     │  │  VIN: 5–26V   │
                                  │  OUT: 5.0V     │  │  (Self-Reg.)   │
                                  └───────┬────────┘  └────────────────┘
                                          │
                                  ┌───────▼────────┐
                                  │  ESP32 DevKit  │
                                  │  VIN = 5.0V    │
                                  │  GND = Common  │
                                  └────────────────┘
```

### 4.3 UART Wiring — ESP32 ↔ Pixhawk (THE Critical Connection)

```
    ESP32 DevKit (30-pin)              Pixhawk TELEM2 (6-pin DF13)
   ┌──────────────────┐               ┌──────────────────┐
   │                  │               │                  │
   │  GPIO16 (RX2) ◄─┼───────────────┼─► TX  (pin 2)   │
   │                  │   CROSS!      │                  │
   │  GPIO17 (TX2) ──►┼───────────────┼─► RX  (pin 3)   │
   │                  │   CROSS!      │                  │
   │  GND ────────────┼───────────────┼── GND (pin 6)   │
   │                  │   COMMON!     │                  │
   └──────────────────┘               └──────────────────┘

   ⚠️ TX connects to RX, RX connects to TX (cross-over)
   ⚠️ GND MUST be connected (common reference)
   ⚠️ Do NOT connect 5V from TELEM2 to ESP32 GPIO pins (3.3V only!)
   ⚠️ Both sides are 3.3V TTL — no level shifter needed
```

**Pixhawk TELEM2 Pinout (DF13 6-pin):**

| Pin | Signal | Connect to ESP32 |
|-----|--------|-----------------|
| 1 | VCC (5V) | **DO NOT CONNECT to GPIO!** Only to VIN if needed |
| 2 | TX | → ESP32 GPIO16 (RX2) |
| 3 | RX | ← ESP32 GPIO17 (TX2) |
| 4 | CTS | Not connected |
| 5 | RTS | Not connected |
| 6 | GND | → ESP32 GND |

---

## Phase 1 — Frame Assembly (45 minutes)

> [!NOTE]
> The F450 frame consists of two center plates (top and bottom — these are the PDB) and four arms.

### Steps

1. **Unbox the F450 kit.** Lay out:
   - 2× center plates (fiberglass PCB with copper traces)
   - 4× arms (2 white, 2 red — or similar color coding)
   - 4× A2212 KV1000 motors (with screws)
   - 4× 30A ESCs (with bullet connectors)
   - 2× pairs 1045 propellers (CW and CCW)
   - Hardware bag (M3 bolts, nuts, standoffs)

2. **Identify the bottom plate.** Look for the solder pads labeled **+** and **−** on the copper traces. This is the PDB (Power Distribution Board).

3. **Attach the four arms to the bottom plate:**
   - Red/colored arms go on opposite sides (front-left and rear-right, or as your kit dictates)
   - Use M3 bolts through the arm holes into the bottom plate holes
   - Finger-tighten only for now — you'll finalize after motor mounting

4. **Mount motors onto arm tips:**
   - Each motor has 4 screw holes matching the arm tip pattern
   - Use the provided M3 screws (usually 4 per motor)
   - Thread-locker (Loctite Blue) is recommended on motor screws
   - **Motor rotation:** Do NOT install props yet, but note:

```
        FRONT
    M1 (CW)    M2 (CCW)
        ╲        ╱
         ╲      ╱
          ╲    ╱
           ╲  ╱
    M4 (CCW) ╳  M3 (CW)
           ╱  ╲
          ╱    ╲
         ╱      ╲
        ╱        ╲
        REAR

    ArduPilot Quad-X Motor Order:
    M1 = Front-Right (CW)   — MAIN OUT 1
    M2 = Rear-Left   (CW)   — MAIN OUT 2  (or as per your ESC calibration)
    M3 = Front-Left  (CCW)  — MAIN OUT 3
    M4 = Rear-Right  (CCW)  — MAIN OUT 4
```

> [!WARNING]
> Motor numbering and rotation direction MUST match ArduPilot's Quad-X configuration. Incorrect mapping will cause an immediate flip on takeoff. Verify in Mission Planner's Motor Test page.

5. **Tighten all frame bolts** with the provided hex key. Check that arms don't wobble.

---

## Phase 2 — Motor & ESC Wiring (2 hours)

> [!CAUTION]
> This phase requires **soldering**. You cannot use breadboard wires for motor power — they will melt and catch fire under 30A loads.

### 2.1 Solder ESC Power Wires to PDB

1. **Pre-tin the PDB pads:** Apply flux, then melt a small blob of solder on each **+** and **−** pad of the bottom center plate.

2. **Pre-tin the ESC power wires:** Strip 3-5mm of insulation from each ESC's red (+) and black (−) power wire. Apply solder to the exposed copper.

3. **Solder each ESC:**
   - Red wire → **+** pad (match to the arm the ESC serves)
   - Black wire → **−** pad
   - Hold for 2-3 seconds until the solder flows smoothly
   - The solder joint should be shiny and smooth — not dull/cold

4. **Heat shrink** each solder joint individually with the 4mm heat shrink tubing.

5. **Route ESC along the arm:** Use zip ties to secure each ESC body underneath its arm. Keep ESC signal wires (thin 3-wire cable) running toward the center.

### 2.2 Connect ESC Signal Wires to Motor Wires

Each ESC has 3 bullet connectors (A, B, C) that connect to the motor's 3 phase wires. Plug them in for now — if a motor spins the wrong direction, swap any two of the three wires.

### 2.3 Connect ESC Signal Cables to Pixhawk

Each ESC has a thin 3-wire servo cable (Signal/VCC/GND). These plug into the Pixhawk **MAIN OUT** pins:

| ESC | Pixhawk MAIN OUT Pin | Motor Position |
|-----|---------------------|----------------|
| ESC 1 | MAIN OUT 1 | Front-Right (CW) |
| ESC 2 | MAIN OUT 2 | Rear-Left (CW) |
| ESC 3 | MAIN OUT 3 | Front-Left (CCW) |
| ESC 4 | MAIN OUT 4 | Rear-Right (CCW) |

> [!IMPORTANT]
> Plug the signal wire so that the **brown/black wire** (GND) faces the edge of the Pixhawk board, and the **white/orange wire** (Signal) faces the center. Check your Pixhawk's silkscreen markings.

---

## Phase 3 — Pixhawk Mounting & Core Wiring (1 hour)

### 3.1 Mount Pixhawk on Frame

1. **Attach the vibration-damping mount** (included with the kit) to the top center plate
2. **Place the Pixhawk** on the damper — the **arrow on the Pixhawk must point FORWARD** (toward the front of the drone)
3. Secure with the provided foam tape or rubber grommets
4. **Do NOT over-tighten** — the dampener must be free to absorb vibration

### 3.2 Connect the Safety Switch

- Plug the safety switch cable into the Pixhawk **SWITCH** port
- Mount the switch somewhere accessible on an arm or the top plate
- You must press and hold this switch to arm the drone (blinking → solid = armed)

### 3.3 Connect the Buzzer

- Plug into the **BUZZER** port
- Mount it so it's not muffled by the frame
- The buzzer provides audible feedback for arming, GPS lock, errors, and low battery warnings

### 3.4 Mount & Connect GPS Module (Neo-M8N)

- **Mount the GPS on a raised mast** (GPS stalk) if provided, or use a foam block to elevate it 5-10cm above the top plate. It must have clear sky view.
- The **GPS arrow must point FORWARD** (same direction as Pixhawk arrow)
- Connect the GPS cable to the Pixhawk **GPS** port (6-pin DF13)
- Connect the compass cable (if separate) to the **I2C** port

### 3.5 Connect Power Module

- Plug the Power Module's **6-pin cable** into the Pixhawk **POWER** port
- The other end of the Power Module has:
  - **XT60 female** → will connect to battery
  - **XT60 male** → will connect to PDB (via your XT60 pigtail)

### 3.6 Insert SD Card

- Insert the provided micro-SD card into the Pixhawk's SD slot
- ArduPilot will log all flight data here

---

## Phase 4 — ESP32 Companion Computer Integration (1 hour)

### 4.1 Prepare the UART Cable

You need 3 wires from the Pixhawk TELEM2 port to the ESP32:

| Wire | From (Pixhawk TELEM2) | To (ESP32) | Color Convention |
|------|----------------------|------------|-----------------|
| TX→RX | TELEM2 Pin 2 (TX) | GPIO16 (RX2) | Green |
| RX←TX | TELEM2 Pin 3 (RX) | GPIO17 (TX2) | Orange |
| GND | TELEM2 Pin 6 (GND) | GND | Black |

**Method A (Bench Testing):** Use DuPont jumper wires from the TELEM2 breakout cable to the ESP32 pins on the breadboard.

**Method B (Final Install):** Solder the 3 wires onto a perfboard with the ESP32, and use the DF13 pigtail cable that came with the Pixhawk kit.

### 4.2 Mount the ESP32

- Use double-sided foam tape to mount the ESP32 on the top plate or inside the frame
- Keep it away from the GPS module (the WiFi antenna can interfere with GPS)
- Ensure the USB port is accessible for firmware updates

### 4.3 Power Wiring for ESP32

The ESP32 gets power from the XL4015 buck converter, NOT from the Pixhawk:

```
F450 PDB (11.1V) ──────► XL4015 IN+ / IN-
                                │
                          Adjusted to 5.0V
                                │
XL4015 OUT+ ────────────► ESP32 VIN pin
XL4015 OUT- ────────────► ESP32 GND pin
```

> [!WARNING]
> **Before connecting the ESP32**, use a multimeter to verify the XL4015 output is **exactly 5.0V ± 0.1V**. Turn the voltage potentiometer slowly with a small screwdriver while measuring. Too high (>5.5V) will damage the ESP32. Too low (<4.5V) will cause brownouts and WiFi failures.

**XL4015 Adjustment Procedure:**
1. Connect a temporary 12V power source to the XL4015 input (e.g., a 12V adapter, or the drone battery)
2. Put your multimeter on the XL4015 output terminals (OUT+ and OUT-)
3. Turn the **voltage potentiometer** (left blue trimpot) clockwise/counterclockwise until the multimeter reads **5.00V**
4. Set the **current potentiometer** (right blue trimpot) to ~2A max (the ESP32 + modem won't need more than 1A)
5. Disconnect the temporary source — the XL4015 will remember its setting

---

## Phase 5 — 4G Modem Integration (30 minutes)

### 5.1 Insert SIM Card

- Insert your active **Nano-SIM** (Jio/Airtel with data plan) into the 7Semi EC200U modem's SIM slot
- The modem will auto-register on the 4G network when powered

### 5.2 Power the Modem

The 7Semi EC200U has a **5–26V wide input range**, so you have two options:

**Option A (Recommended): Direct from PDB**
- Solder the modem's VIN and GND wires directly to the F450 PDB pads
- The modem's internal regulator handles 11.1V safely
- This keeps its power path completely separate from the ESP32, preventing cellular TX burst brownouts

**Option B: From XL4015 output**
- Connect the modem's VIN to the same 5V output as the ESP32
- Simpler wiring but may cause ESP32 brownouts during 4G transmission bursts (the EC200U can draw up to 2A peaks during cellular transmission)

### 5.3 Connect Modem to ESP32

The 7Semi EC200U communicates over **USB-C** or **UART**. For Skylink:

**Current Firmware Architecture:** The Skylink firmware currently uses **WiFi** to create a network connection for the GCS browser. The modem provides the **internet uplink** — it creates a hotspot or routes traffic via USB tethering. The ESP32 connects to the modem's WiFi hotspot, and remote GCS clients connect through the 4G link.

**Wiring:** Connect the modem's USB-C port to a USB breakout, or use the modem's UART pins for AT command configuration. The exact integration depends on your chosen network topology:

| Topology | How It Works | Modem Wiring |
|----------|-------------|--------------|
| **WiFi Bridge** (Simplest) | Modem creates a WiFi hotspot. ESP32 connects to it as a client. Your phone/laptop connects to the same hotspot or via the 4G internet. | USB-C power only. Configure hotspot via AT commands over USB. |
| **USB Tethering** | Modem tethers internet to a WiFi router. ESP32 connects to that router. | USB-C to router/phone. |
| **Direct UART** | ESP32 sends AT commands directly to the modem for internet access (PPP/MQTT). | UART TX/RX from modem to ESP32 spare UART pins. |

> [!NOTE]
> For your initial setup, the **WiFi Bridge** topology is simplest. Configure the modem to create a WiFi hotspot, add it to your `wifi_networks.json`, and the ESP32 will auto-connect.

### 5.4 Mount the Modem

- Mount using foam tape on the frame, away from the GPS module
- If using the external GPS antenna for the modem's GNSS, route the u.FL antenna cable to a clear-sky position
- Keep the modem's cellular antenna clear of carbon fiber and metal (if applicable)

---

## Phase 6 — Power Distribution & Battery (1 hour)

### 6.1 Complete Power Wiring

Solder all remaining power connections to the F450 PDB:

```
PDB Pad (+) ───► XL4015 IN+
PDB Pad (-) ───► XL4015 IN-

PDB Pad (+) ───► 7Semi Modem VIN (if using Option A)
PDB Pad (-) ───► 7Semi Modem GND
```

### 6.2 XT60 Connector Wiring

The Power Module has two XT60 connectors:
- **Battery side (female):** Plugs directly into the LiPo battery
- **Load side (male):** Connects to the PDB

Use your **XT60 male pigtail connector** to create the bridge from the Power Module's load output to the F450 PDB:

```
LiPo Battery ──(XT60)──► Power Module ──(XT60)──► F450 PDB ──► ESCs + Buck + Modem
                              │
                        6-pin cable to Pixhawk POWER port
                        (voltage + current sensing)
```

### 6.3 Insulate All Exposed Connections

- Apply **heat shrink** to every solder joint
- Use **electrical tape** on any exposed copper
- Double-check for shorts between + and − with your multimeter in continuity mode

### 6.4 Attach the Top Center Plate

- Once all wiring is routed through the center, place the top plate over the bottom plate
- Secure with the provided standoff screws
- Route all wires neatly — nothing should touch props or be loose enough to vibrate into propeller path

---

## Phase 7 — Firmware & Software Setup (2 hours)

### 7.1 Flash ArduCopter to Pixhawk

1. Connect Pixhawk to your PC via **Micro-USB**
2. Open **Mission Planner** (Windows) or **QGroundControl**
3. Go to **Setup → Install Firmware**
4. Select **ArduCopter** (Quad) — latest stable version
5. Wait for flash to complete (~2 minutes)

> [!IMPORTANT]
> **Flash ArduPilot (ArduCopter), NOT PX4.** The Skylink firmware is written specifically for ArduPilot's MAVLink mode definitions (GUIDED, LOITER, RTL, STABILIZE). PX4 uses different mode/command semantics and is NOT compatible.

### 7.2 Set Critical ArduPilot Parameters

In Mission Planner: **Config → Full Parameter List**. Set these parameters:

#### TELEM2 / SERIAL2 (ESP32 Link)

| Parameter | Value | Notes |
|-----------|-------|-------|
| `SERIAL2_PROTOCOL` | **2** | MAVLink 2 protocol |
| `SERIAL2_BAUD` | **115** | 115200 baud (matches ESP32 `fcSerial.begin(115200, SERIAL_8N1, 16, 17)`) |

#### Telemetry Stream Rates (SERIAL2)

| Parameter | Value | Notes |
|-----------|-------|-------|
| `SR2_POSITION` | **2** | GPS position at 2 Hz |
| `SR2_EXTRA1` | **10** | Attitude at 10 Hz |
| `SR2_EXT_STAT` | **2** | Battery/status at 2 Hz |

#### Frame Configuration

| Parameter | Value | Notes |
|-----------|-------|-------|
| `FRAME_CLASS` | **1** | Quad |
| `FRAME_TYPE` | **1** | X configuration |

#### Internet-Only (No RC) Safety Parameters

| Parameter | Value | Notes |
|-----------|-------|-------|
| `FS_THR_ENABLE` | **0** | Disable throttle failsafe (no RC transmitter) |
| `FS_GCS_ENABLE` | **1** | RTL if GCS heartbeat lost for 5 seconds |
| `ARMING_CHECK` | Exclude **RC** | Skip RC calibration check for arming |

#### Barometer Fix (Robokits-Specific)

| Parameter | Value | Notes |
|-----------|-------|-------|
| `BARO_OPTIONS` | **1** | **CRITICAL for Robokits kit!** Forces MS5607 barometer scaling. Without this, altitude readings will be wrong and the drone will oscillate in altitude hold. |

#### Safety Limits

| Parameter | Value | Notes |
|-----------|-------|-------|
| `RTL_ALT` | **1500** | RTL altitude = 15 meters (value in centimeters) |
| `FENCE_ENABLE` | **1** | Enable geofence |
| `FENCE_RADIUS` | **300** | 300 meter circular geofence |
| `FENCE_ALT_MAX` | **50** | Maximum altitude 50m |

**Write → Save → Power cycle the Pixhawk.**

### 7.3 Flash Skylink Firmware to ESP32

1. Open the Skylink project in VS Code with PlatformIO

2. **Remove the SITL_MODE flag** from `platformio.ini`:

```ini
; platformio.ini — HARDWARE MODE
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
upload_speed = 115200
upload_port = COM6

board_build.filesystem = littlefs

lib_deps =
  mathieucarbou/AsyncTCP @ ^3.0.0
  mathieucarbou/ESP Async WebServer @ ^3.0.0
  bblanchon/ArduinoJson @ ^7.0.0
  okalachev/MAVLink @ ^2.0.29

build_flags =
  -D WS_MAX_QUEUED_MESSAGES=64
  ; NOTE: -D SITL_MODE is REMOVED for hardware flight!
```

> [!TIP]
> **Best practice:** Create two PlatformIO environments so you can switch without editing:
> ```ini
> [env:esp32dev_sitl]
> extends = env:esp32dev
> build_flags = ${env:esp32dev.build_flags} -D SITL_MODE
>
> [env:esp32dev_hw]
> extends = env:esp32dev
> build_flags = ${env:esp32dev.build_flags}
> ```
> Then flash with:
> - Simulation: `pio run -e esp32dev_sitl --target upload`
> - Hardware: `pio run -e esp32dev_hw --target upload`

3. **Update `wifi_networks.json`** to include the 4G modem's WiFi hotspot:

```json
{
  "networks": [
    {
      "ssid": "EC200U-Hotspot",
      "password": "your_modem_password",
      "priority": 1
    },
    {
      "ssid": "Poojan-G",
      "password": "12345677",
      "priority": 2
    }
  ]
}
```

4. **Upload firmware:**
   ```
   pio run --target upload
   ```

5. **Upload filesystem (LittleFS):**
   ```
   pio run --target uploadfs
   ```

6. **Open Serial Monitor** (115200 baud). You should see:
   ```
   ================================
   Skylink ESP32 Starting
   ================================
   Initializing FlightController in [HARDWARE MODE] via UART2
   ```
   (If you see `[SITL MODE]`, the SITL_MODE flag is still active — rebuild.)

---

## Phase 8 — Bench Testing (NO PROPS)

> [!CAUTION]
> **ALL PROPELLERS MUST BE REMOVED** during bench testing. A spinning propeller on a bench can cause severe cuts and injuries. Perform ALL of the following tests with propellers OFF.

### 8.1 Power-On Sequence

1. Connect the ESP32 to your PC via USB (for serial monitoring)
2. Connect the Pixhawk to the Power Module
3. Connect the LiPo battery to the Power Module
4. You should hear the Pixhawk boot tones (musical beeps)
5. The safety switch should start blinking red
6. The GPS module's LED should start blinking (searching for satellites)

### 8.2 Verify ESP32 ↔ Pixhawk MAVLink Link

1. Open your browser and navigate to `http://<ESP32_IP>/`
2. The Skylink GCS dashboard should load
3. Check the **MAV** indicator — it should turn **GREEN** when heartbeats are received from the Pixhawk
4. Verify telemetry appears:
   - Battery voltage (should read ~11.1V for a 3S battery, or 12.6V for a fully charged 3S)
   - Attitude (roll/pitch/yaw should respond to tilting the Pixhawk)
   - GPS (will show "No Fix" indoors — this is normal)

### 8.3 Verify Motor Directions (Mission Planner)

1. Connect Mission Planner via **USB** (not the ESP32 UART!)
2. Go to **Setup → Motor Test**
3. Test each motor individually at low throttle (5-10%)
4. Verify each motor spins in the correct direction per the Quad-X diagram
5. If a motor spins backwards, **swap any two of the three bullet connectors** between that ESC and motor

### 8.4 Calibrations in Mission Planner

| Calibration | Location | Notes |
|-------------|----------|-------|
| Accelerometer | Setup → Accel Calibration | Follow 6-position calibration (level, left, right, nose up, nose down, upside down) |
| Compass | Setup → Compass Calibration | Do outdoors, away from metal/electronics. Rotate drone in all axes. |
| ESC | Setup → ESC Calibration | Follow ArduPilot procedure — all-at-once method |
| Radio (if using RC) | Setup → Radio Calibration | Move all sticks to extremes and back to center |

### 8.5 Verify GPS Lock (Outdoors)

1. Take the drone outdoors with a clear sky view
2. Wait 2-5 minutes for the GPS to acquire satellites
3. In the Skylink dashboard, verify:
   - GPS Fix ≥ 3 (3D fix)
   - Satellite count ≥ 8
   - Map shows your correct location

### 8.6 Test Arm/Disarm from Skylink

1. Ensure GPS fix ≥ 3
2. Press and hold the **safety switch** until it goes solid
3. In the Skylink GCS, send the **ARM** command
4. The Pixhawk should report armed (you'll hear a tone)
5. **The motors will NOT spin** because props are removed and throttle is at minimum — this is expected
6. Send **DISARM** to verify disarming works

---

## Phase 9 — First Flight Operations

> [!CAUTION]
> **ONLY proceed to this phase after ALL bench tests pass.** First flights should be in an open field, away from people, buildings, and power lines. Have a fire extinguisher nearby. A second person as a spotter is highly recommended.

### 9.1 Pre-Flight Setup

1. Choose an open field (minimum 50m × 50m clear area)
2. Check weather: no rain, wind < 15 km/h
3. Ensure LiPo battery is fully charged (12.6V for 3S)
4. Install propellers:
   - CW props on CW motors (M1, M2)
   - CCW props on CCW motors (M3, M4)
   - Tighten prop nuts firmly — props must NOT be loose

### 9.2 Flight Sequence

1. **Place drone on level ground**, GPS facing up
2. **Power on** — wait for GPS lock (solid green on GPS, Fix ≥ 3 on dashboard)
3. **Open Skylink GCS** on your phone/laptop
4. **Hold safety switch** until solid
5. **Set mode to GUIDED** from the GCS
6. **ARM** the drone — motors will start spinning at idle
7. **TAKEOFF** to 3 meters (send takeoff command with altitude = 3)
8. Observe the hover — is it stable?
9. **Small MOVE_BODY test** — move 1 meter forward
10. **LOITER** — the drone should hold position
11. **RTL** — the drone should climb to RTL altitude, fly back to takeoff point, and land
12. **DISARM** (auto-disarm after landing, or manual from GCS)

### 9.3 Emergency Procedures

| Situation | Action |
|-----------|--------|
| Drone drifting uncontrollably | Hit **LAND** in GCS immediately |
| Internet connection drops | ArduPilot will auto-RTL after 5 seconds (if `FS_GCS_ENABLE = 1`) |
| Drone flips on takeoff | Motor order or rotation is wrong — rebuild motor mapping |
| Battery alarm (buzzer beeping) | Immediately **LAND** — do not fly below 10.5V for 3S |
| Fire/smoke | Disconnect battery immediately, use fire extinguisher |

---

## 10. Critical ArduPilot Parameters

Quick reference table — all parameters that must be set:

| Parameter | Value | Category |
|-----------|-------|----------|
| `SERIAL2_PROTOCOL` | 2 | ESP32 Link |
| `SERIAL2_BAUD` | 115 | ESP32 Link |
| `SR2_POSITION` | 2 | Telemetry Rate |
| `SR2_EXTRA1` | 10 | Telemetry Rate |
| `SR2_EXT_STAT` | 2 | Telemetry Rate |
| `FRAME_CLASS` | 1 | Quad Frame |
| `FRAME_TYPE` | 1 | X Configuration |
| `FS_THR_ENABLE` | 0 | No-RC Safety |
| `FS_GCS_ENABLE` | 1 | GCS Failsafe (RTL) |
| `ARMING_CHECK` | Exclude RC | No-RC Safety |
| `BARO_OPTIONS` | 1 | Robokits Barometer Fix |
| `RTL_ALT` | 1500 | Safety (15m) |
| `FENCE_ENABLE` | 1 | Geofence |
| `FENCE_RADIUS` | 300 | Geofence (300m) |
| `FENCE_ALT_MAX` | 50 | Max Altitude (50m) |

---

## 11. Troubleshooting Reference

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| MAV indicator stays RED | TX/RX wires swapped, or wrong baud rate | Swap GPIO16↔GPIO17 wires; verify `SERIAL2_BAUD = 115` |
| MAV flickers green/red | Loose GND wire, or intermittent connection | Resolder GND; use solid wire, not stranded |
| No GPS fix indoors | GPS needs open sky | Test outdoors with clear view of sky |
| Arm denied: "Check Battery" | LiPo voltage too low | Charge battery to 12.6V (3S) |
| Arm denied: "PreArm: RC not calibrated" | RC failsafe still enabled | Set `FS_THR_ENABLE = 0` and `ARMING_CHECK` to exclude RC |
| Arm denied: "Compass not calibrated" | Compass uncalibrated | Run compass calibration in Mission Planner |
| Altitude oscillation | Wrong barometer driver | Set `BARO_OPTIONS = 1` (MS5607 fix for Robokits kit) |
| Drone flips on takeoff | Wrong motor order or rotation | Remove props, verify in Mission Planner Motor Test |
| ESP32 shows `[SITL MODE]` | SITL_MODE still in build flags | Remove `-D SITL_MODE` from `platformio.ini`, rebuild |
| ESP32 brownout / reset | XL4015 voltage wrong, or modem drawing too much current | Verify 5.0V on multimeter; power modem separately from PDB |
| Dashboard loads but no data | ESP32 not connected to WiFi, or wrong IP | Check serial monitor for WiFi connection status |
| Commands sent but nothing happens | Not in GUIDED mode, or not armed | Set mode to GUIDED, then ARM |
| Map click rejected | Beyond 1000m geofence (firmware limit) | Click closer to home position |

---

## 12. Pre-Flight Checklist

Print this and check before every flight:

### ✅ Hardware Checks
- [ ] All propellers tight and correct (CW/CCW matched to motors)
- [ ] Battery fully charged (12.6V for 3S)
- [ ] Battery securely strapped (velcro or strap)
- [ ] All connectors firm (XT60, ESC bullets, TELEM2, GPS)
- [ ] No loose wires in propeller path
- [ ] GPS module has clear sky view
- [ ] SD card inserted in Pixhawk

### ✅ Software Checks
- [ ] ESP32 firmware is **HARDWARE MODE** (not SITL)
- [ ] WiFi connected (check serial monitor)
- [ ] GCS dashboard loads in browser
- [ ] MAV indicator GREEN
- [ ] GPS Fix ≥ 3, Satellites ≥ 8
- [ ] Battery voltage reading correct on dashboard
- [ ] Attitude responds to tilting drone

### ✅ Safety Checks
- [ ] Open area, no people within 30m
- [ ] Wind < 15 km/h
- [ ] `FS_GCS_ENABLE = 1` (auto-RTL on link loss)
- [ ] Fire extinguisher nearby
- [ ] Spotter/second person present
- [ ] Know your emergency procedure (LAND or RTL)

### ✅ Go/No-Go
- [ ] All above checks PASS → **CLEAR TO FLY** ✈️

---

## Associated Technical References

| Document | Link |
|----------|------|
| Skylink Final BOM Report | [skylink_final_bom_report.md](file:///d:/btp_skylink/Skylink/docs/components/final_reports/skylink_final_bom_report.md) |
| Cellular Modems & Alternatives | [cellular_modems_and_premium_alternatives.md](file:///d:/btp_skylink/Skylink/docs/components/final_reports/cellular_modems_and_premium_alternatives.md) |
| Pixhawk Hardware Roadmap | [PIXHAWK_HARDWARE_ROADMAP.md](file:///d:/btp_skylink/Skylink/docs/PIXHAWK_HARDWARE_ROADMAP.md) |
| Pixhawk 2.4.8 Combo Kit | [pixhawk_248_robokits_combo.md](file:///d:/btp_skylink/Skylink/docs/components/kits/pixhawk_248_robokits_combo.md) |
| F450 Frame Kit | [f450_quadcopter_frame_kit.md](file:///d:/btp_skylink/Skylink/docs/components/kits/f450_quadcopter_frame_kit.md) |
| 7Semi EC200U Industrial Modem | [7semi_ec200u_mini_industrial_modem.md](file:///d:/btp_skylink/Skylink/docs/components/networking/7semi_ec200u_mini_industrial_modem.md) |
| ESP32 Component Reference | [ESP32_Development_Board_30Pin.md](file:///d:/btp_skylink/Skylink/docs/components/WHAT_I_ALREADY_HAVE/ESP32_Development_Board_30Pin.md) |
| XL4015 Buck Converter Reference | [XL4015_Buck_Converter_Module.md](file:///d:/btp_skylink/Skylink/docs/components/WHAT_I_ALREADY_HAVE/XL4015_Buck_Converter_Module.md) |

---

> *Last updated: May 26, 2026 — Skylink FW 12 / FS 15, ArduCopter 4.x*
