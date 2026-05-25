# Pixhawk 2.4.8 Full Combo Set — Component Research & Reference

> [!NOTE]
> This reference document details the components included in the **Pixhawk 2.4.8 Full Combo Set** from Evelta. It lists each item's physical role, connection details, and significance to the **Skylink** companion computer bridge ecosystem.
>
> **Product URL:** [Evelta Pixhawk 2.4.8 Full Combo Set](https://evelta.com/pixhawk-2-4-8-px4-autopilot-32-bit-flight-controller-for-rc-drone-uav/)
> **Last Updated:** May 25, 2026

---

## 1. Overview of the Kit

The **Pixhawk 2.4.8 Full Combo Set** is an all-in-one hardware package designed to build a fully autonomous unmanned aerial vehicle (UAV). When paired with the **Skylink ESP32 MAVLink bridge**, this hardware forms the complete physical stack of the drone, enabling remote control from a web dashboard.

Below is the detailed breakdown of what each included component does, how it interfaces with the autopilot, and how it contributes to the Skylink ecosystem.

---

## 2. Component Directory

```
                     ┌───────────────────────────┐
                     │   Pixhawk 2.4.8 Brain     │
                     └─────────────┬─────────────┘
          ┌────────────────────────┼────────────────────────┐
          ▼                        ▼                        ▼
    [Neo-M8N GPS]           [Power Module]            [OLED Display]
   (Compass/Location)       (Voltage/Sensors)         (Bench telemetry)
          │                        │                        │
          ▼                        ▼                        ▼
     [GPS Mast]             [LiPo Battery]           [I2C Splitter]
    (Isolate GPS)          (Primary Power)          (Multiple Sensors)
```

### 1. Pixhawk 2.4.8 Flight Controller (FC)
*   **What it is:** The central computer or "brain" of the drone.
*   **Detailed Function:** It houses a 32-bit ARM Cortex-M4 processor running the NuttX Real-Time Operating System (RTOS) and runs the **ArduPilot (ArduCopter)** autopilot software. It contains internal Inertial Measurement Units (IMUs: Accelerometers, Gyros, Magnetometers, and a Barometer). It continuously reads sensor data, executes stabilizing PID loops, receives pilot steering inputs (via RC receiver), and sends PWM signals to the Electronic Speed Controllers (ESCs) to govern motor speeds.
*   **Skylink Interfacing:** It communicates with the ESP32 (running Skylink bridge firmware) over its `TELEM2` port via **MAVLink 2** protocol (at 115200 baud). Skylink sends high-level commands (e.g. `GUIDED` takeoff, `GOTO` coordinate hops) and receives real-time telemetry back from this board.

### 2. Neo-M8N GPS Module
*   **What it is:** A high-precision Global Navigation Satellite System (GNSS) receiver.
*   **Detailed Function:** It tracks multiple satellite constellations (GPS, GLONASS, BeiDou) to compute the drone’s precise 3D position (latitude, longitude, altitude) and velocity. Crucially, the housing also contains an **external digital compass (magnetometer)**.
*   **Skylink Interfacing:** The GPS is connected to the `GPS` port (DF13 5-pin/6-pin) and the I2C bus (for the compass). GPS data is read by the Pixhawk and streamed back to the Skylink ESP32 via MAVLink. This coordinate data is what places the drone icon on your **Skylink Leaflet-based GCS map** and permits coordinates to be sent via click-to-fly (`GOTO_LATLON`).

### 3. Power Module (PM)
*   **What it is:** A dual-purpose power regulator and sensor unit.
*   **Detailed Function:** 
    1.  **Regulator:** Connects directly to the main LiPo flight battery (supporting 2S–6S) and steps down the raw voltage to a clean, filtered 5.3V to power the Pixhawk FC and basic telemetry modules.
    2.  **Sensor:** Measures real-time battery voltage and current consumption.
*   **Skylink Interfacing:** Plugs into the `POWER` port on the Pixhawk. The measured voltage and current are read by ArduPilot and packaged into standard MAVLink messages (e.g. `SYS_STATUS`), which Skylink relays via WebSockets to show the **live Battery Voltage and Remaining %** on the GCS dashboard.

### 4. Buzzer
*   **What it is:** An audio alarm indicator.
*   **Detailed Function:** Plays distinct audio frequencies and patterns to communicate drone status to the pilot when a screen is not visible. It plays boot-up tunes, calibration alarms, sensor error tones, GPS status notes, and arming alerts.
*   **Skylink Interfacing:** Plugs directly into the dedicated `BUZZER` port. For example, when you send an `ARM` command from your Skylink dashboard, the buzzer will emit a long tone signaling that the motors are spinning up.

### 5. Safety Switch
*   **What it is:** A push-button switch with a built-in status LED.
*   **Detailed Function:** Serves as a physical hardware safety interlock. By default, ArduPilot blocks electrical signal transmission to the motors even if the drone is armed in software. The operator must physically press and hold this safety button on the drone for 3 seconds to make the motors live. 
*   **Skylink Interfacing:** Plugs into the dedicated `SWITCH` port. The flashing light becomes a solid red when safety is deactivated. This physical safety step is critical to prevent accidental throttle triggers while handling the drone on the bench.

### 6. Shock Absorber Board
*   **What it is:** A mechanical vibration isolation platform.
*   **Detailed Function:** Consists of two fiberglass plates separated by four soft rubber dampening balls. High-frequency vibrations from spinning brushless motors and propellers can travel through the frame and overwhelm the Pixhawk's internal gyros/accelerometers, causing erratic flight, altitude drift, or severe flyaways.
*   **Skylink Interfacing:** The Pixhawk must be mounted on top of this dampening plate using double-sided tape, and the plate is then mounted to the drone frame. Mechanical vibration isolation is mandatory to ensure clean telemetry readings on the Skylink dashboard.

### 7. GPS Holder / Mast
*   **What it is:** An elevated mounting rod (usually folding carbon fiber or plastic).
*   **Detailed Function:** Mounts the Neo-M8N GPS several inches above the main drone frame.
*   **Skylink Interfacing:** The GPS module contains the magnetometer (compass). Placing it close to high-current power cables, ESCs, or the LiPo battery exposes it to electromagnetic fields that distort compass readings, leading to "toilet-bowling" flight paths. Raising the GPS keeps the heading reading accurate on the Skylink GCS map.

### 8. I2C Splitter Board
*   **What it is:** A bus expansion hub.
*   **Detailed Function:** The I2C protocol is a daisy-chain bus where multiple sensors can share a single SDA/SCL channel. Since the Pixhawk 2.4.8 only has one physical I2C port, this hub splits it into 4–5 parallel plugs.
*   **Skylink Interfacing:** Connects to the `I2C` port on the Pixhawk. This allows you to simultaneously plug in the **Neo-M8N Compass line**, the **OSD OLED Display**, and other optional I2C sensors (such as digital airspeed sensors or lidar).

### 9. OSD OLED Display
*   **What it is:** A micro organic LED screen (usually 128x64 pixels, SSD1306-driven).
*   **Detailed Function:** Displays basic diagnostics directly on the drone. It prints the active flight mode, battery voltage, GPS status, calibration errors, and boot codes.
*   **Skylink Interfacing:** Plugs into the I2C splitter. It helps developers inspect the drone's telemetry state on the bench without needing to open Mission Planner on a PC or power up the Skylink WiFi dashboard.

### 10. PPM Encoder
*   **What it is:** A pulse-width modulation (PWM) to pulse-position modulation (PPM) signal converter.
*   **Detailed Function:** Classic RC receivers output a separate signal wire for every control channel (throttle, roll, pitch, yaw). The Pixhawk, however, expects a single serial stream (PPM or SBUS) via a single wire. This encoder takes up to 8 separate channel wires from an older receiver and compresses them into a single PPM output wire.
*   **Skylink Interfacing:** Connects between your traditional RC receiver outputs and the Pixhawk’s `RC IN` port. (If using a modern SBUS, IBUS, or ELRS receiver, this encoder is not required, as those connect directly with a single wire).

### 11. SD Card
*   **What it is:** A high-speed MicroSD flash memory card.
*   **Detailed Function:** Plugs directly into the Pixhawk’s built-in card slot. ArduPilot uses the SD card to:
    1.  Store and load system parameters (`.param` configurations).
    2.  Write detailed high-frequency flight logs (`.bin` logs) for post-flight telemetry analysis.
    3.  Cache terrain altitude profiles for low-altitude waypoint missions.
*   **Skylink Interfacing:** The flight logs stored on this card can be retrieved using Mission Planner via USB to analyze flight dynamics and command latency after testing a series of Skylink guided commands.

### 12. RGB Module
*   **What it is:** An external, super-bright, multicolor LED.
*   **Detailed Function:** Mirrors the color-coded flashing indicator lights of the central LED on the Pixhawk itself.
*   **Skylink Interfacing:** Connects to the I2C splitter. Because the Pixhawk flight controller is typically protected inside a canopy or shock mount under a pile of wires, this external LED can be mounted on the outer edge of the drone so the operator can clearly verify flight readiness (e.g. flashing blue for ready, green for GPS lock, yellow for failsafe).

### 13. DF13 Connection Cables
*   **What they are:** Standard multi-conductor wire harnesses.
*   **Detailed Function:** The Pixhawk 2.4.8 uses Hirose DF13-series ports (with 4, 5, or 6 pins). These cables are used to interconnect all the elements listed above to the correct sockets on the Pixhawk housing.

---

## 3. How to Wire the Complete Skylink Hardware Stack

```
        ┌──────────────────┐               ┌──────────────────┐
        │  ESP32 Companion │               │  Pixhawk 2.4.8   │
        │      (Skylink)   │               │      (Autopilot) │
        ├──────────────────┤               ├──────────────────┤
        │      GPIO 16 (RX)◄───────────────┤ TELEM2 TX        │
        │      GPIO 17 (TX)├───────────────► TELEM2 RX        │
        │      GND         ├───────────────┤ TELEM2 GND       │
        └──────────────────┘               └──────────────────┘
                                                    ▲
                                                    │
                                           ┌────────┴────────┐
                                           │  I2C Splitter   │
                                           └─┬─────────────┬─┘
                                             │             │
                                             ▼             ▼
                                       [Neo-M8N Compass] [OLED Display]
```

1.  **Mount the Pixhawk** on top of the **Shock Absorber Board** with the arrow pointing forward (towards the front of your quadcopter frame).
2.  **Mount the GPS Mast** on the frame, and secure the **Neo-M8N GPS** on top of the mast with the arrow also pointing forward.
3.  **Plug the GPS connector** into the `GPS` port, and connect the GPS's I2C 4-pin connector into the **I2C Splitter Board**.
4.  **Connect the I2C Splitter** into the `I2C` port on the Pixhawk. Plug the **OLED Display** and the **RGB LED** into the remaining slots on the splitter.
5.  **Connect the Buzzer** to the `BUZZER` port, and the **Safety Switch** to the `SWITCH` port.
6.  **Connect the Power Module** between your LiPo battery and the drone’s power distribution board. Connect the 6-pin PM cable into the `POWER` port on the Pixhawk.
7.  **Connect the Skylink ESP32:**
    *   Solder a custom 3-wire cable (TX, RX, GND) with a 6-pin DF13 connector matching the Pixhawk's `TELEM2` pinout.
    *   **Wiring Scheme:** 
        *   Pixhawk TELEM2 **TX** ──► ESP32 **GPIO 16** (RX2)
        *   Pixhawk TELEM2 **RX** ──► ESP32 **GPIO 17** (TX2)
        *   Pixhawk TELEM2 **GND** ──► ESP32 **GND**
8.  **Powering the ESP32:** You can power the ESP32 via a USB cable from a small power bank or by connecting the 5V line from TELEM2 to the ESP32's `VIN` pin (only if the Pixhawk regulator provides stable current).
