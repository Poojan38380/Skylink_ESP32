# Waveshare SIM7600G-H 4G Dongle with GNSS — Component Research & Reference

> [!NOTE]
> This reference document details the technical specifications, pinout configurations, and integration procedures for the **Waveshare SIM7600G-H 4G Dongle with GNSS Global Band Support (SKU: TIFCC0236)** sold by Robocraze. It highlights its role, power considerations, and hardware wiring in the context of the **Skylink** companion computer (ESP32) and Pixhawk autopilot ecosystem.
>
> **🚧 STATUS: FUTURE / NOT YET IMPLEMENTED IN FIRMWARE**
> The ESP32 firmware does **not** currently have AT-command drivers or 4G telemetry routing code. This doc is research-only for future BVLOS expansion. Do not purchase this dongle until firmware support is implemented (planned post-Pixhawk Phase H5).
>
> **Product URL:** [Robocraze Waveshare SIM7600G-H 4G Dongle](https://robocraze.com/products/waveshare-sim7600g-h-4g-dongle-with-gnss-global-band-support)
> **Retail Price:** ₹5,196 INR (inc. GST, 20% off from ₹6,499)
> **Last Updated:** May 26, 2026

---

## 1. Overview & System Relevance

The **Waveshare SIM7600G-H 4G Dongle** is a high-performance, industrial-grade cellular modem that integrates multi-constellation GNSS positioning (GPS, BeiDou, GLONASS, Galileo). Supported by global 2G, 3G, and 4G LTE frequency bands, it provides universal connectivity for outdoor robotics, telemetry, and IoT platforms.

### 📶 A Game-Changer for Skylink: Beyond Visual Line of Sight (BVLOS)
In standard setups, the Skylink ESP32 companion computer communicates with the Ground Control Station (GCS) browser dashboard via a **local point-to-point Wi-Fi network**. While excellent for bench testing and close-range flights, local Wi-Fi is strictly limited by distance (typically under 100 meters in real-world interference-heavy environments).

By integrating the SIM7600G-H 4G Dongle, you can elevate Skylink from local Wi-Fi to a **Cellular Telemetry & Control Link**:
* **Unlimited Range (BVLOS):** The drone sends telemetry and receives commands over standard cellular networks. As long as the drone and operator's laptop/phone have cellular coverage, Skylink can be operated across cities or even continents.
* **Redundant Positioning (GNSS):** The dongle includes an onboard GNSS engine. This can serve as a secondary, independent positioning source for telemetry logs or fail-safe operations if the Pixhawk’s primary GPS module (e.g., Neo-M8N) suffers from interference or failure.
* **Dual Interface Support:** Features a direct USB port for X86/ARM companion computers (e.g., Raspberry Pi) and a dedicated 6-pin UART interface with level translation, making it plug-and-play for microcontrollers like the ESP32.

---

## 2. Technical Specifications & Frequency Bands

The dongle utilizes the highly reliable SimCom SIM7600G-H cellular engine, featuring the following hardware capabilities:

### Core Specifications

| Hardware Attribute | Value / Capability | Functional Role |
| :--- | :--- | :--- |
| **Cellular Engine** | **SIM7600G-H** | Global-band 4G LTE Cat-4 module with 2G/3G fallbacks. |
| **Downlink Speed** | **Up to 150 Mbps** | Maximum downstream bandwidth for large data/video relays. |
| **Uplink Speed** | **Up to 50 Mbps** | Maximum upstream bandwidth for sending telemetry/coordinates to GCS. |
| **GNSS Capabilities** | **GPS, BeiDou, GLONASS, Galileo, LBS** | Integrated multi-constellation receiver for high-precision tracking. |
| **Baud Rate Support** | **300 bps to 4 Mbps** (Default: 115200 bps) | Communication speed over UART interface; supports auto-negotiation. |
| **SIM Card Slot** | **Nano-SIM (1.8V and 3V)** | Standard micro-sized SIM slot with push-push ejection design. |
| **Onboard USB Port** | **USB 2.0 Micro-B** | Used for PC, Raspberry Pi, or Jetson Nano companion connections. |
| **Onboard UART Port** | **6-Pin Header (3.3V Logic)** | Serial communication interface for microcontrollers (ESP32/STM32). |

### Global Band Support

The SIM7600G-H is designed to work in any region globally by supporting the following bands:

```
                            ┌───────────────────────────────────┐
                            │    SIM7600G-H Band Support        │
                            └─────────────────┬─────────────────┘
         ┌─────────────────────────┬──────────┴──────────┬─────────────────────────┐
         ▼                         ▼                     ▼                         ▼
   [LTE-FDD Bands]           [LTE-TDD Bands]       [WCDMA Bands]              [GSM Bands]
   B1, B2, B3, B4, B5,       B34, B38, B39,        B1, B2, B4, B5,            B2, B3, B5, B8
   B7, B8, B12, B13, B18,    B40, B41              B6, B8, B19                (850/900/1800/1900 MHz)
   B19, B20, B25, B26, B28, B66
```

---

## 3. Interfaces, Pinouts, & Onboard Indicators

### 6-Pin UART Header Pinout
For direct integration with the Skylink ESP32 companion board, the dongle provides a dedicated 6-pin UART interface. The onboard circuitry translates the module's native low-voltage lines to a safe **3.3V logic level**, making level shifters unnecessary.

```
       ┌───────┐
       │ (1) 5V│  ◄── Power Input (5V DC, min 1A, supports high bursts)
       │ (2)GND│  ◄── Ground (Must connect to host common GND)
       │ (3)TXD│  ◄── Transmit Data (Output to host RXD, 3.3V logic)
       │ (4)RXD│  ◄── Receive Data (Input from host TXD, 3.3V logic)
       │ (5)CTS│  ◄── Clear To Send (Hardware flow control output, optional)
       │ (6)RTS│  ◄── Request To Send (Hardware flow control input, optional)
       └───────┘
```

| Pin # | Silk Label | Signal Direction | Hardware Description |
| :---: | :---: | :---: | :--- |
| **1** | **5V** | Input (Power) | Power supply input. Feeds the onboard step-down buck converter. **Must be able to supply up to 2A transient spikes.** |
| **2** | **GND** | Reference | System Ground. Must share a common ground with the ESP32 and flight controller. |
| **3** | **TXD** | Output (from Dongle) | Serial Transmit line (3.3V logic level). Connects to ESP32 RX pin. |
| **4** | **RXD** | Input (to Dongle) | Serial Receive line (3.3V logic level). Connects to ESP32 TX pin. |
| **5** | **CTS** | Output (from Dongle) | Hardware flow control: Clear to Send. Prevents data overflow (optional). |
| **6** | **RTS** | Input (to Dongle) | Hardware flow control: Request to Send. Signals host readiness (optional). |

### 3x LED Operational Indicators
The dongle features three surface-mount LEDs to monitor system and network conditions in real time:

1. **PWR (Power Indicator - Solid Red):**
   * **State:** Solid ON when power is supplied to the 5V line and the internal voltage regulators are functioning.
   * **State:** OFF indicates lack of power, or voltage drops below operational thresholds.
2. **STA (Status Indicator - Solid Green):**
   * **State:** ON indicates the internal SIM7600 processor has successfully booted and the operating system firmware is fully initialized.
   * **State:** OFF indicates the processor is in a low-power sleep state or has not finished booting.
3. **NET (Network Status Indicator - Blue/Yellow):**
   * **Fast Flashing (200ms ON / 200ms OFF):** Active 4G LTE cellular data connection (PPP/NDIS data transmission active).
   * **Slow Flashing (800ms ON / 800ms OFF):** Registered on a cellular network (2G, 3G, or 4G) but currently in an idle state.
   * **Always ON:** Searching for a cellular carrier network or active in a voice call.
   * **OFF:** No network registered, SIM card missing/invalid, or device is powered off.

---

## 4. Skylink Integration Scenarios

There are two primary ways to incorporate the SIM7600G-H 4G Dongle into the Skylink system architecture.

### Option A: ESP32-as-Gateway (Recommended for Skylink Web UI)
In this topology, the ESP32 continues to act as the primary bridge. However, instead of broadcasting a local Wi-Fi access point, the ESP32 handles AT commands to initialize the SIM7600G-H, establishes a TCP/IP or WebSocket socket connection back to a cloud server (which acts as a relay), and tunnels MAVLink packets back and forth.

```
┌─────────────────┐             ┌─────────────────┐             ┌─────────────────┐
│  Pixhawk 2.4.8  │ ◄──UART1──► │   ESP32 DevKit  │ ◄──UART2──► │   SIM7600G-H    │
│  (Autopilot)    │             │   (Companion)   │             │   (4G Dongle)   │
└─────────────────┘             └─────────────────┘             └────────┬────────┘
                                                                         │
                                                                       4G LTE
                                                                         │
                                                                         ▼
                                                               ┌─────────────────┐
                                                               │  Cloud Relay /  │
                                                               │  Browser GCS    │
                                                               └─────────────────┘
```

#### Wiring Connections (ESP32 ↔ SIM7600G-H)
Connect the dongle's UART header directly to the ESP32's available hardware serial pins:

```
        ESP32 DevKit                         Waveshare SIM7600G-H
     ┌───────────────┐                    ┌─────────────────────────┐
     │ GPIO 22 (RX1) ◄────────────────────┤ Pin 3 (TXD)             │
     │ GPIO 23 (TX1) ├────────────────────► Pin 4 (RXD)             │
     │ Ground (GND)  ├────────────────────┤ Pin 2 (GND)             │
     │ External 5V   ├────────────────────► Pin 1 (5V)              │
     └───────────────┘                    └─────────────────────────┘
```
*(Note: Pin definitions are examples. Verify platformio.ini config or script overrides to match physical GPIO mappings).*

### Option B: Direct Pixhawk Telemetry Link (ArduPilot Native)
If the companion computer is an ARM host (like a Raspberry Pi or Jetson Nano running Skylink on-board), the dongle can plug directly into the host's USB port. Alternatively, the dongle's UART can connect directly to the Pixhawk's `TELEM1` or `TELEM2` port, allowing ArduPilot's native cellular scripting framework to establish a transparent connection to a cellular telemetry server.

```
        Pixhawk 2.4.8                        Waveshare SIM7600G-H
     ┌────────────────┐                   ┌─────────────────────────┐
     │ Pin 2 (TXD)    ├──────────────────►│ Pin 4 (RXD)             │
     │ Pin 3 (RXD)    ◄───────────────────┤ Pin 3 (TXD)             │
     │ Pin 4 (RTS)    ├──────────────────►│ Pin 6 (RTS)             │
     │ Pin 5 (CTS)    ◄───────────────────┤ Pin 5 (CTS)             │
     │ Pin 6 (GND)    ├───────────────────┤ Pin 2 (GND)             │
     └────────────────┘                   └─────────────────────────┘
```
*(Note: Pins refer to standard Pixhawk DF13 6-pin TELEM connector ports).*

---

## 5. Critical Hardware Rules & Power Guide

Cellular communication devices are notorious for their highly erratic current draw patterns, which can severely compromise flight electronics if not handled correctly:

### ⚠️ Power Isolation & Peak Currents
* > [!CAUTION]
  > **High Burst Currents:** When registering on a tower or transmitting data packets, the SIM7600G-H can draw transient current spikes **exceeding 2.0 Amps**.
  >
  > **Never power the SIM7600G-H from the ESP32's 3.3V or 5V out pins, nor from the Pixhawk's internal telemetry port power rails.** Doing so will cause severe voltage brownouts, resetting the ESP32 and triggering flight processor crashes or critical failsafes in the middle of a flight.
* **Solution:** Power the SIM7600G-H via a dedicated, heavy-duty **5V buck regulator** (rated for at least 3.0A continuous output) connected directly to the main power distribution board (PDB).

### 📡 Antenna Installation & Safety
* > [!IMPORTANT]
  > **Never Power On Without Antennas:** Operating cellular modules without the LTE antenna connected will cause the RF power amplifiers on the SIM7600G-H chip to overheat, leading to permanent hardware degradation or immediate failure.
  >
  > **Antenna Spacing:** Mount the 4G LTE antenna as far away as possible from the Pixhawk's main GPS/Magnetometer module (minimum 15-20cm spacing). High-power RF emissions from the 4G antenna can flood the GNSS receiver frontend, causing loss of GPS satellite locks and degrading EKF3 stability.
* **GNSS Antenna:** The onboard GNSS engine requires an **active external GNSS antenna** connected to the micro-coaxial (IPEX/U.FL) connector on the dongle's PCB. An extra antenna is required to make positioning functional, as the included 360-degree antenna is designed for cellular LTE frequencies only.

---

## 6. ArduPilot (Mission Planner) Parameters

If connecting the SIM7600G-H directly to a Pixhawk serial port (e.g., `TELEM2` as Serial 2) for cellular telemetry, apply the following configuration parameters inside **Mission Planner**:

| Parameter | Recommended Value | Detailed Purpose |
| :--- | :--- | :--- |
| **`SERIAL2_PROTOCOL`** | **2** (or **48**) | Configures Serial 2 for MAVLink 2 protocol, or PPP network mode if running ArduPilot scripts. |
| **`SERIAL2_BAUD`** | **115** | Sets serial baud rate to 115200 (must match SIM7600G-H's baud rate). |
| **`BRD_SER2_RTSCTS`** | **1** | Enables Hardware Flow Control (RTS/CTS). **Highly recommended** to prevent buffer overflow and packet loss over cellular links. |
| **`SCR_ENABLE`** | **1** | Enables ArduPilot Lua scripting (required if running LTE cellular helper scripts). |

After writing these parameters, save the settings in Mission Planner and power-cycle the flight controller to activate the serial and script engines.

---

## 7. Package Checklist

When receiving the Robocraze package (SKU: TIFCC0236), verify that all the following components are accounted for before assembly:

* `[ ]` **1 x SIM7600G-H 4G DONGLE** (Core cellular & GNSS transceiver unit in protective enclosure).
* `[ ]` **1 x 360 Degree Foldable LTE Antenna** (Multi-band cellular antenna).
* `[ ]` **1 x USB Extension Cable** (For PC/Companion computer USB testing).
* `[ ]` **1 x 6PIN Jumper Wire** (For direct connection to ESP32 / Pixhawk UART ports).
* `[ ]` **1 x Screws Tool Kit** (For mounting, enclosure adjustments, and securing jumpers).
* `[ ]` **Active Nano-SIM Card** (Purchased separately with active data plan, e.g., Jio, Airtel).
* `[ ]` **Active GNSS/GPS External Antenna** (Purchased separately; required for positioning features).
