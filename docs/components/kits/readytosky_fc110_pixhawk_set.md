# ReadytoSky Pixhawk 2.4.8 Flight Controller Set — Component Research & Reference

> [!NOTE]
> This reference document details the components, onboard silicon chips, and technical specifications of the **ReadytoSky Pixhawk 2.4.8 Flight Controller Set (SKU: 447-FC-110)** sold by Evelta.
>
> **Product URL:** [Evelta ReadytoSky Pixhawk 2.4.8 Autopilot Set](https://evelta.com/pixhawk-2-4-8-px4-autopilot-32-bit-flight-controller-for-rc-drone-uav/)
> **Retail Price:** ₹10,342.50 INR (inc. GST)
> **Last Updated:** May 25, 2026

---

## 1. Overview & Difference from Full Combo Kit

This set represents the **Standard Autopilot Base Set** manufactured by ReadytoSky (MPN: FC-110). It provides the core flight computer, safety peripherals, power supply, and dampening accessories required for stabilized flight.

### ⚠️ Critical Difference for Skylink
Unlike the "Full Combo Set" which includes a GPS module, this set **DOES NOT include a GPS module (like the Neo-M8N) or a GPS holder/mast**.
*   **For Stabilized Flight:** This kit can fly the quadcopter in manual modes (like `STABILIZE`, `ACRO`, and `ALT_HOLD`).
*   **For Skylink Autonomous Flight:** You **must purchase an external GPS+Compass module** (such as a Neo-M8N or Holybro M10 GPS) separately. Without a GPS, Skylink's map dashboard will not show the drone's position, and advanced modes (`GUIDED`, `GOTO_LATLON`, `RTL`) will be blocked by the safety pre-arm checks in the ArduPilot firmware.

---

## 2. Technical Specifications & Onboard Silicon

This flight controller is built on a highly reliable, dual-processor redundant architecture. Understanding the exact chips on the board is crucial for technical documentation and troubleshooting:

### Onboard Microcontrollers & Memory

| Technical Specification | Value | Hardware Role & Purpose |
| :--- | :--- | :--- |
| **Main Processor** | **STM32F427 Cortex M4** | The primary CPU. It runs the real-time operating system (NuttX RTOS) and executes the ArduPilot (ArduCopter) flight control loops, sensor fusion algorithms (EKF3), and MAVLink communications. |
| **Clock Speed** | **168 MHz** | High-speed clock frequency ensuring microsecond-level precision for flight motor adjustments. |
| **Flash Memory** | **2 MB** | High-capacity flash storage containing the ArduPilot/PX4 compiled binary firmware. |
| **RAM** | **256 KB** | Static RAM for running real-time flight calculations and maintaining transaction buffers. |
| **Co-Processor** | **STM32F103** | A dedicated backup/failsafe processor. If the primary STM32F427 hangs or crashes, this co-processor takes immediate control to execute emergency landing procedures or pass manual RC commands straight to the motors. |

### Onboard Sensors (IMUs & Barometer)
The Pixhawk 2.4.8 board contains multiple redundant inertial and pressure sensors to measure the drone's posture, heading, and altitude:

```
                  ┌───────────────────────────────┐
                  │      Pixhawk 2.4.8 Board      │
                  └───────────────┬───────────────┘
         ┌────────────────┬───────┴────────┬───────────────┐
         ▼                ▼                ▼               ▼
   [L3GD20 Gyro]    [LSM303D Accel]  [MPU6000 IMU]   [MS5611 Baro]
    (Rotational)     (Magnetic/Acc)   (Vibe-Resist)   (Altitude/Air)
```

1.  **MPU6000 6-Axis IMU (Inertial Measurement Unit):**
    *   **Silicon:** Invensense MPU6000 (combines a 3-axis accelerometer and a 3-axis gyroscope on a single chip).
    *   **Role:** Known for its high mechanical vibration resistance. It serves as the primary sensor for measuring pitch, roll, and yaw rates, allowing the autopilot to counteract wind gusts and maintain stability.
2.  **L3GD20 3-Axis Gyroscope:**
    *   **Silicon:** STMicroelectronics L3GD20.
    *   **Role:** Measures angular velocity (rotational speed around the pitch, roll, and yaw axes). Serves as a redundant sensor to crosscheck the gyroscope readings from the MPU6000.
3.  **LSM303D Accelerometer & Magnetometer:**
    *   **Silicon:** STMicroelectronics LSM303D.
    *   **Role:** Combines a 3-axis accelerometer (to measure linear gravity vectors) and a 3-axis geomagnetic sensor (digital compass). The onboard magnetometer acts as the primary backup compass if the external GPS module's compass fails or encounters magnetic interference.
4.  **MS5611 Barometer:**
    *   **Silicon:** MEAS Switzerland MS5611 high-precision barometric pressure sensor.
    *   **Role:** Measures ambient air pressure changes. It is sensitive enough to detect altitude shifts of less than 10 centimeters, making it the primary sensor used to maintain altitude hold (`ALT_HOLD` mode) when a GPS is not active.

---

## 3. Peripheral Interfaces & Ports
The board features extensive connectivity to integrate with companion computers (like Skylink) and RC receivers:
*   **5x UART Ports (Serial):** Telemetry 1 (`TELEM1`), Telemetry 2 (`TELEM2` — **used for Skylink ESP32**), GPS/I2C port, Serial 4/5 (dual port), and Console.
*   **2x CAN Interfaces:** Controlled Area Network ports for industrial UAV peripherals (like professional ESCs or smart batteries).
*   **Receiver Support:** Multi-protocol support including **DSM/DSMX, SBUS, and PPM**.
*   **Communication Buses:** Dedicated SPI, I2C, and UART connections for plug-and-play expansions.

---

## 4. Packing List Breakdown

This specific kit (FC-110) from ReadytoSky contains 9 physical components. Here is what each item does and where it plugs:

| # | Component | Functional Role | Connection Port on Pixhawk |
| :--- | :--- | :--- | :--- |
| **1** | **Pixhawk 2.4.8 FC** | The autopilot hardware containing the STM32 processors and IMUs. | *N/A (Central Hub)* |
| **2** | **Buzzer** | Plays diagnostic sound patterns to notify pre-arm states, low battery, and system locks. | `BUZZER` (2-pin DF13) |
| **3** | **Safety Switch** | A physical button that must be pressed for 3 seconds to activate the motor PWM lines. | `SWITCH` (3-pin DF13) |
| **4** | **Power Module** | Dual-role 5.3V voltage regulator and battery current/voltage measurement module. | `POWER` (6-pin DF13) |
| **5** | **RGB Connector** | An external multicolor LED module that mirrors the color codes of the internal status LED. | `I2C` Bus |
| **6** | **I2C Extension** | A cable / mini-board to connect digital I2C devices like compasses and external displays. | `I2C` Port |
| **7** | **Shock Absorber Mount** | Rubber-isolated platform to mechanically isolate the FC from frame vibrations. | *Physical Frame Mount* |
| **8** | **SD Card** | Plugs into the FC slot to store system parameters and flight logs. | *SD Card Slot (MicroSD)* |
| **9** | **PPM Encoder** | Signal translator board to convert old multi-wire RC receivers to a single PPM stream. | `RC IN` Port |

---

## 5. Skylink Integration & Safe Operating Guide

If you purchase this specific ReadytoSky kit, follow these steps to connect your Skylink ESP32 bridge:

### 1. Connecting the ESP32 to TELEM2
Make the physical cross-connections between your ESP32 companion board and the Pixhawk's `TELEM2` port using a DF13 6-pin cable:

```
        ESP32 DevKit                       Pixhawk 2.4.8 (TELEM2)
     ┌───────────────┐                    ┌─────────────────────────┐
     │ GPIO 16 (RX2) ◄────────────────────┤ Pin 2 (TX)              │
     │ GPIO 17 (TX2) ├────────────────────► Pin 3 (RX)              │
     │ Ground (GND)  ├────────────────────┤ Pin 6 (GND)             │
     └───────────────┘                    └─────────────────────────┘
```

### 2. Mandatory Add-on: External GPS
Because this specific SKU (447-FC-110) does not include a GPS module, you **must source one separately**.
*   **Recommended Module:** **Neo-M8N GPS with Compass** (approximately ₹1,800 - ₹2,500 INR standalone).
*   **GPS Connection:** Plug the main GPS cable into the `GPS` port (DF13 6-pin) and the separate 4-pin compass cable into the `I2C` port (or the included I2C extension board).
*   **Note:** Without this GPS, you will be unable to run coordinates-based autonomous routines in the Skylink dashboard.
