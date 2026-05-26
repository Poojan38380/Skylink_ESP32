# Pixhawk 2.4.8 Basic GPS Combo Kit — Component Research & Reference

> [!NOTE]
> This reference document details the components, onboard silicon differences, and integration procedures for the **Pixhawk 2.4.8 Basic Flight Controller kit with GPS Module Combo Kit (Model: RKI-7227)** sold by Robokits India. It highlights their role, critical parameter adjustments, and hardware wiring in the context of the **Skylink** companion computer (ESP32) and Pixhawk autopilot ecosystem.
>
> **Product URL:** [Robokits Pixhawk 2.4.8 GPS Combo Kit](https://robokits.co.in/multirotor-spare-parts/flight-controller-and-frame/pixhawk-2.4.8-basic-flight-controller-kit-with-gps-module-combo-kit)
> **Retail Price:** ₹7,379.00 INR (Excl. GST) / ₹8,707.22 INR (Incl. 18% GST)
> **Last Updated:** May 26, 2026

---

## 1. Overview & Difference from Evelta Combo Set

The **Robokits Pixhawk 2.4.8 GPS Combo Kit (Model: RKI-7227)** is a streamlined hardware bundle designed to provide the core autopilot, stabilization, and positioning essentials for a drone. 

### ⚠️ Critical Component Differences for Skylink Developers
Before integrating this kit with your Skylink ESP32 bridge, you must be aware of how this kit differs from the **Evelta Full Combo Set** to avoid setup errors:

| Component Feature | Robokits Basic Combo (RKI-7227) [This Kit] | Evelta Full Combo Set | Skylink Impact & Workarounds |
| :--- | :--- | :--- | :--- |
| **Barometer Sensor** | **MEAS MS5607** | MEAS MS5611 | **CRITICAL:** The MS5607 has a different resolution and calibration conversion math. Standard firmware expects MS5611. You **must** set ArduPilot parameters or you will experience severe altitude drift and pre-arm failures (see Section 2). |
| **I2C Splitter Board** | **NOT INCLUDED** | Included (4-port hub) | Without a splitter, the single I2C port on the Pixhawk is fully occupied by the **Neo-M8N Compass**. If you want to add an OLED display or external I2C LED, you must purchase a splitter separately. |
| **OLED Display** | **NOT INCLUDED** | Included (SSD1306) | You will not have bench diagnostics directly on the airframe. Status checking must be performed via the **Skylink GCS Web Dashboard** or PC USB Mission Planner. |
| **External RGB LED** | **NOT INCLUDED** | Included | You must rely on the Pixhawk's central internal status LED, which can be hard to see once mounted under cables or canopies. |
| **PPM Encoder** | **NOT INCLUDED** | Included | If you are using an older RC receiver with individual channel outputs, you must buy an encoder. Modern SBUS/IBUS/ELRS receivers do not need this. |

---

## 2. 🚨 Critical Tuning: The MS5607 Barometer Substitution

The Pixhawk 2.4.8 board included in this Robokits bundle utilizes the **MEAS Switzerland MS5607** barometric pressure sensor rather than the standard **MS5611** found in traditional Pixhawk designs.

Because these two sensors share identical pins and I2C/SPI addresses, the board will boot normally. However, because they require different mathematical scaling to convert raw pressure to altitude, **ArduPilot will calculate completely incorrect altitude readings out of the box.**

### 🛑 Symptoms of Wrong Barometer Calibration
* The drone reports rapid altitude climbs or drops while resting static on the bench.
* Pixhawk fails pre-arm safety checks with errors like: `PreArm: Baro: GPS alt error` or `PreArm: Baro sensor unhealthy`.
* Altitude Hold (`ALT_HOLD`) and Loiter (`LOITER`) modes lead to immediate flyaways or ground crashes because the autopilot misinterprets air pressure.

### 🛠️ Step-by-Step Fix in Mission Planner
To resolve this issue and enable stable flight, you must force ArduPilot to utilize the MS5607 driver:

1. Connect the Pixhawk to your PC via a USB cable.
2. Open **Mission Planner** and click **Connect** (top right).
3. Navigate to **Configuration** ──► **Full Parameter List**.
4. Search for the parameter **`BARO_OPTIONS`**.
5. Change the value of `BARO_OPTIONS` from `0` to **`1`** (this bitmask forces ArduPilot to treat MS5611 instances as MS5607 and applies the correct conversion formulas).
6. Click **Write Params** to save.
7. Disconnect the USB cable and power-cycle the flight controller.
8. Verify that the HUD altitude in Mission Planner and the **Skylink Dashboard** stabilizes and correctly reads `0.0m` (or relative altitude) at ground level.

---

## 3. Onboard Silicon & Component Directory

The Robokits combo features a robust, dual-processor redundant architecture:

```
                            ┌────────────────────────┐
                            │      RKI-7227 Combo    │
                            └───────────┬────────────┘
            ┌───────────────────────────┼───────────────────────────┐
            ▼                           ▼                           ▼
     [Pixhawk 2.4.8 FC]        [uBlox Neo-M8N GPS]          [XT60 Power Module]
     (STM32F427 / F103)         (GNSS & Compass)            (5.3V / 3A BEC PM)
            │                           │                           │
            ▼                           ▼                           ▼
     [Vibe Shock Mount]        [Folding GPS Mast]           [Buzzer & Switch]
    (Mechanical Damper)         (EMI Isolation)             (Physical Safety)
```

### Onboard Microcontrollers & Memory

| Technical Specification | Value | Hardware Role & Purpose |
| :--- | :--- | :--- |
| **Main Processor** | **STM32F427 Cortex M4** | Runs the real-time operating system (NuttX RTOS) and executes ArduPilot flight loops, sensor fusion (EKF3), and MAVLink telemetry streams to the Skylink ESP32. |
| **Co-Processor** | **STM32F103** | Failsafe co-processor. Executes emergency override landing loops if the primary F427 encounters a software crash. |
| **Clock Speed / Flash** | **168 MHz / 2 MB** | High-performance MCU core with massive storage for advanced Ardupilot firmware. |
| **RAM** | **128 KB SRAM** | Low-latency static RAM for real-time transaction buffers and sensor processing. |

### Onboard Sensor Suite (IMUs & Barometer)
* **Invensense MPU6000 IMU:** Combines a 3-axis accelerometer and 3-axis gyroscope. Highly resistant to high-frequency mechanical vibration.
* **ST Micro L3GD20H Gyroscope:** High-performance 16-bit gyroscope for redundant angular rate detection.
* **ST Micro X4HBA 303H Accel/Magnetometer:** Equivalent to LSM303D. Provides redundant accelerometer and a secondary compass on-board.
* **MEAS MS5607 Barometer:** High-precision barometric pressure sensor (requires `BARO_OPTIONS = 1` as detailed in Section 2).

---

## 4. Combo Package Components

This specific kit (RKI-7227) contains 5 physical modules. Here is their role and where they plug:

### 1. Pixhawk PX4 2.4.8 Autopilot (RKI-1491)
* **Role:** The primary flight controller unit.
* **Connection:** Serves as the central hub. Connects to motors via MAIN OUT and ESP32 via TELEM2.

### 2. uBlox Neo-M8N GPS with Compass (RKI-4060)
* **Role:** Concurrent GNSS module (GPS, GLONASS, BeiDou) that streams location coordinates to Pixhawk. Houses the external magnetometer (compass) used as the primary heading sensor.
* **Connection:** 
  * **GPS Signal:** Plugs into the `GPS` port (6-pin DF13).
  * **Compass Signal:** Plugs directly into the Pixhawk’s `I2C` port (4-pin DF13). Since no I2C splitter is included, this occupies the entire I2C bus.

### 3. GPS Base Folding Antenna Pack (RKI-1466)
* **Role:** Elevated folding carbon fiber rod (14cm length) and CNC aluminum alloy mount.
* **Connection:** Mechanically mounted to the quadcopter frame. Elevates the Neo-M8N compass away from electromagnetic interference (EMI) generated by ESCs and high-current wires.

### 4. Pixhawk 2.4.8 APM 2.8 Power Module (RKI-1478)
* **Role:** Steps down battery voltage (supports up to 6S / 28V) to 5.3V @ 3A to power the Pixhawk. Measures overall battery voltage and current consumption up to 90A.
* **Connection:** Plugs into the `POWER` port (6-pin DF13) using the included cable. Battery connects to the XT60 input, and output XT60 runs to the Power Distribution Board (PDB).

### 5. Anti-Vibration Shock Absorber Kit (RKI-1916)
* **Role:** A dual-plate glass fiber mount suspended by 4 soft rubber dampening balls.
* **Connection:** Plugs directly under the Pixhawk housing. Protects the internal MPU6000 and sensors from high-frequency frame vibrations that would otherwise distort EKF3 calculations and induce toilet-bowling.

---

## 5. Wiring the Kit to the Skylink ESP32 Companion Computer

Because this kit **lacks an I2C splitter board and an OLED display**, the physical and logical wiring layout is simpler than the full Evelta combo:

```
        ESP32 DevKit                         Pixhawk 2.4.8 (TELEM2)
     ┌───────────────┐                    ┌─────────────────────────┐
     │ GPIO 16 (RX2) ◄────────────────────┤ Pin 2 (TX)              │
     │ GPIO 17 (TX2) ├────────────────────► Pin 3 (RX)              │
     │ Ground (GND)  ├────────────────────┤ Pin 6 (GND)             │
     └───────────────┘                    └─────────────────────────┘
                                                       
        uBlox Neo-M8N                        Pixhawk 2.4.8 (GPS & I2C)
     ┌───────────────┐                    ┌─────────────────────────┐
     │ GPS Cable     ├────────────────────► GPS Port (6-Pin DF13)    │
     │ Compass Cable ├────────────────────► I2C Port (4-Pin DF13)    │
     └───────────────┘                    └─────────────────────────┘
```

### Physical Connections Setup
1. **Mounting:** Secure the Pixhawk to the top plate of the **Anti-Vibration Shock Absorber Kit (RKI-1916)** using double-sided adhesive tape. Mount the shock absorber securely to the center of your drone frame (e.g., F450 frame bottom plate).
2. **GPS Setup:** Assemble the **Folding GPS Mast (RKI-1466)** and mount it to the frame. Secure the **Neo-M8N GPS (RKI-4060)** on the top plate. Ensure the arrow on the GPS module matches the forward-facing arrow on the Pixhawk.
3. **GPS Wiring:**
   * Plug the **6-pin GPS cable** into the `GPS` port.
   * Plug the **4-pin Compass cable** directly into the single `I2C` port. (No splitter board is needed, as the compass is the only I2C device).
4. **Power Wiring:** Connect the **Power Module (RKI-1478)** between your 3S/4S LiPo battery and the PDB. Connect the 6-pin cable from the PM to the `POWER` port on the Pixhawk.
5. **Skylink Bridge (ESP32):**
   * Solder a 3-wire serial cable (TX, RX, GND) with a 6-pin DF13 connector.
   * Connect Pixhawk `TELEM2` **TX** ──► ESP32 **GPIO 16** (RX2).
   * Connect Pixhawk `TELEM2` **RX** ──► ESP32 **GPIO 17** (TX2).
   * Connect Pixhawk `TELEM2` **GND** ──► ESP32 **GND** (Common Ground).
6. **ESP32 Powering:** Power the ESP32 via its micro-USB port from a clean 5V source, or connect a 5V buck regulator directly to the ESP32's `VIN` pin.

---

## 6. Pre-Flight Calibration Checklist for Skylink

Once the physical wiring is completed, go through this calibration checklist to ensure autonomous flight modes work correctly on your Skylink dashboard:

* `[ ]` **Barometer Option Fix:** Set `BARO_OPTIONS = 1` in Mission Planner to scale the MS5607 sensor correctly, then reboot the Pixhawk (crucial for altitude hold and guided modes).
* `[ ]` **Accelerometer Calibration:** Perform the 3D axis calibration in Mission Planner (placing the drone on its left, right, nose, tail, back, and level).
* `[ ]` **Compass Calibration:** Conduct the outdoor compass calibration away from metallic structures. Verify that the heading indicator on your **Skylink GCS Leaflet Map** accurately changes as you rotate the drone frame.
* `[ ]` **Power Module Calibration:** Inside Mission Planner, select **Battery Monitor** ──► **Sensor: Other** ──► **APM 2.5 Power Module**. This guarantees that the live battery voltage displayed on the Skylink dashboard is accurate and prevents premature failsafes.
* `[ ]` **ESC & Radio Calibration:** Calibrate your transmitter sticks and ESCs on the bench (**with propellers removed**). Verify motor rotations are CCW for motors 1 & 2, and CW for motors 3 & 4.
