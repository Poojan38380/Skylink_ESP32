# Pixhawk 2.4.8 Configuration Guide for Skylink

This document serves as the master reference guide for configuring, calibrating, and troubleshooting a **Pixhawk 2.4.8 flight controller** running **ArduCopter V3.6.8** inside the **Skylink** autonomous companion drone ecosystem. 

Follow this guide if the flight controller undergoes a parameter factory reset, hardware swap, or when setting up a new build.

---

## 1. System Overview & Communication Path

The ESP32 acts as a cellular/WiFi companion computer to the Pixhawk. It is NOT the autopilot. The flight controller runs the real-time flight controls, while the ESP32 relays telemetry and translates high-level coordinates into flight path directions.

```
 ┌──────────────────────┐          ┌──────────────────┐          ┌─────────────────┐
 │  Operator Dashboard  │  ──WiFi──►  ESP32 Companion  │  ──UART──►  Pixhawk 2.4.8  │
 │     (Browser GCS)    │  ◄──Web──  │   (Skylink FW)   │  ◄──MAV──  │ (ArduCopter3.6) │
 └──────────────────────┘          └──────────────────┘          └────────┬────────┘
                                                                          │
                                                                 ┌────────┴────────┐
                                                                 │  I2C Splitter   │
                                                                 └──┬───┬───────┬──┘
                                                                    │   │       │
                                                 ┌──────────────────┘   │       └─────────────────┐
                                                 ▼                      ▼                         ▼
                                         [Neo-M8N Compass]       [RGB LED Module]       [I2C OLED Display]
```

---

## 2. Step-by-Step Parameter Configuration

After flashing **ArduCopter V3.6.8** and performing a factory parameter reset, open **Mission Planner**, navigate to **Config/Tuning → Full Parameter List**, and apply the following settings.

### A. Frame Configuration
These parameters tell the flight controller the physical layout of the quadcopter. Without these, the motors will not spin during arming tests.
*   **`FRAME_CLASS`** = `1` (Quad)
*   **`FRAME_TYPE`** = `1` (X-frame configuration)

### B. TELEM2 / SERIAL2 Interface (ESP32 MAVLink Connection)
Sets up the serial communication between the ESP32 DevKit and the Pixhawk's `TELEM2` hardware port.
*   **`SERIAL2_PROTOCOL`** = `2` (MAVLink 2 protocol)
*   **`SERIAL2_BAUD`** = `115` (115200 bps baud rate; must match the ESP32 code)
*   **`SR2_POSITION`** = `2` (Streams GPS position data at 2 Hz)
*   **`SR2_EXTRA1`** = `10` (Streams attitude data [roll/pitch/yaw] at 10 Hz)
*   **`SR2_EXT_STAT`** = `2` (Streams battery metrics and system status at 2 Hz)

### C. Battery & Power Monitor Configuration
Enables real-time analog voltage and current monitoring, feeding telemetry data to the OLED screen and the GCS dashboard.
*   **`BATT_MONITOR`** = `4` (Voltage and Current monitoring)
*   **`BATT_VOLT_PIN`** = `2` (Pixhawk hardware voltage ADC pin)
*   **`BATT_CURR_PIN`** = `3` (Pixhawk hardware current ADC pin)
*   **`BATT_VOLT_MULT`** = `10.10101` (Standard voltage multiplier for the 3DR Power Module)
*   **`BATT_AMP_PERVOLT`** = `18.0018` (Standard current calibration multiplier)

> [!NOTE]
> Alternatively, you can use the visual wizard in Mission Planner: Go to **Initial Setup → Optional Hardware → Battery Monitor** and set:
> * **Monitor:** `Analog Voltage and Current`
> * **Sensor:** `4: 3DR Power Module`
> * **HW Ver:** `4: The Cube or Pixhawk`
> * **Battery Capacity:** `5200` mAh (or matches your exact LiPo capacity)

### D. Peripheral Screen & RGB LED Configuration
Enables the I2C OLED display and external status RGB LED.
*   **`NTF_DISPLAY_TYPE`** = `1` (Enables the SSD1306 OLED driver; note that Copter 4.x+ uses `DISPLAY_TYPE`)
*   **`NTF_LED_TYPES`** = `199` (Enables the external I2C RGB LED along with internal notifications)

### E. Bench Testing Safety Overrides (No RC Mode)
When testing the drone on the bench **WITHOUT PROPELLERS**, use these overrides to prevent failsafes from blocking motor tests or arming:
*   **`BRD_SAFETYENABLE`** = `0` (Disables the hardware safety switch button so you can arm over software)
*   **`FS_THR_ENABLE`** = `0` (Disables the RC transmitter receiver throttle failsafe)
*   **`FS_GCS_ENABLE`** = `1` (Configures flight controller to Return-to-Launch [RTL] if the GCS telemetry link is lost for 5 seconds)

> [!WARNING]
> **Safety parameters must be restored to secure values before actual outdoor flights!** Never fly outdoors without a bound RC transmitter, and ensure `FS_THR_ENABLE` is turned back on.

*Remember to click **Write Params** and reboot the Pixhawk after updating parameters!*

---

## 3. Physical Port Pinout & Wiring Guides

### 3.1 TELEM2 Port (ESP32 UART Link)
Connect the ESP32 hardware UART2 pins to the Pixhawk's `TELEM2` port. Pins are ordered from **left to right** (looking directly at the connector port from the front, with the Pixhawk top-cover facing up):

| Pin (Left-to-Right) | Function | Wire Color (Typical JST-GH/DF13) | ESP32 Pin Connection |
| :---: | :---: | :---: | :---: |
| **1** | **VCC (5V)** | Red | **Do Not Connect** (Keep disconnected to prevent power loops) |
| **2** | **TX (Out)** | Orange | **GPIO 16 (RX2)** |
| **3** | **RX (In)** | Yellow | **GPIO 17 (TX2)** |
| **4** | **CTS** | Green | *Leave Disconnected* (Hardware flow control not used) |
| **5** | **RTS** | Blue | *Leave Disconnected* (Hardware flow control not used) |
| **6** | **GND** | Black | **GND** (Common ground reference) |

> [!IMPORTANT]
> **Power isolation:** The ESP32 is powered via the XL4015 buck converter, and the Pixhawk is powered by the Power Module. **Never connect Pin 1 (VCC 5V) of TELEM2 to the ESP32** while either device is plugged into USB or powered by the battery. This prevents feedback currents that can blow out regulator ICs.

### 3.2 I2C Splitter & OLED Display Wiring
Because the standard Pixhawk JST-DF13 I2C connector and the SSD1306 OLED pin headers have mismatched ordering, you must connect them using individual female-to-female DuPont jumper wires.

* **I2C Splitter Socket Pinout (Left to Right):**
  1. `VCC` (5V / Red)
  2. `SCL` (Clock / Green)
  3. `SDA` (Data / Yellow)
  4. `GND` (Ground / Black)

* **OLED Header Pinout (Silkscreened on board):**
  `VCC` | `GND` | `SCL` | `SDA`

#### Cross-Wiring Table:
Connect the wires from the JST-DF13 cable (plugged into the splitter) to the OLED pins as follows:

| JST Cable Core | Splitter Signal | Wire Color | Destination OLED Pin |
| :---: | :---: | :---: | :---: |
| Pin 1 | **VCC** | Red | **VCC** |
| Pin 2 | **SCL** | Green / Blue | **SCL** |
| Pin 3 | **SDA** | Yellow | **SDA** |
| Pin 4 | **GND** | Black | **GND** |

---

## 4. Diagnostic & Troubleshooting Trees

### 🪓 Problem 1: ESP32 reads `Cannot arm — no MAVLink link to autopilot`
1.  **Check Serial Monitor:** Ensure the ESP32 baud rate is 115200.
2.  **Verify Pixhawk Parameters:** Open Mission Planner over USB. Verify `SERIAL2_BAUD` is set to `115` and `SERIAL2_PROTOCOL` is set to `2`.
3.  **Confirm TX/RX Cross-Wiring:** Swap the connections on **GPIO 16** and **GPIO 17** on the ESP32 side. A common mistake is connecting TX to TX and RX to RX.
4.  **Resolve Windows COM Port Conflict:**
    *   If PlatformIO throws `Access is denied` or `GetOverlappedResult failed`, you have another software (likely Mission Planner or Cura) trying to open the ESP32's COM port.
    *   Ensure Mission Planner is connected to the **Pixhawk COM Port**, not the ESP32 COM Port. Disconnect Mission Planner completely before running `pio device monitor`.

### 🪓 Problem 2: OLED Screen remains blank
1.  **Verify Parameter:** Ensure `NTF_DISPLAY_TYPE` is set to `1` (SSD1306).
2.  **Power Cycle:** The OLED driver initializes *only during the Pixhawk boot phase*. If you plug the screen in while the flight controller is already powered, it will stay blank. Reboot the Pixhawk.
3.  **Check SCL/SDA Pin alignment:** Double-check that `SCL` goes to `SCL` and `SDA` goes to `SDA`. Swapping these two I2C lines prevents communication, but does not damage the screen.
4.  **Test I2C Splitter Port:** Plug the display cable into a different port on the I2C splitter. If one port has broken physical connections, other ports will still work (since they are all in parallel).

### 🪓 Problem 3: Battery reads `0.00 V` on screen / GCS
1.  **Check the Battery State:** Make sure a LiPo battery is physically plugged into the Power Module XT60 connector. Powering only over USB does not activate the battery voltage sensing rail.
2.  **Verify Parameter:** Confirm `BATT_MONITOR` is set to `4` (Voltage and Current).
3.  **Check 6-pin cable:** Make sure the Power Module cable is plugged into the port labeled **`POWER`** on the Pixhawk, not `TELEM1` or `TELEM2`.
