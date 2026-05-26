# Skylink Final BOM Analysis & Integration Report

> [!NOTE]
> This comprehensive report evaluates the compatibility of your selected drone components, details the architecture for **internet-only/cellular GCS flight**, incorporates your existing electronic components (ESP32, XL4015 buck converter), and lists the remaining required parts to successfully bring your autonomous web-controlled F450 quadcopter to life.
>
> **Research Date:** May 26, 2026  
> **Prepared for:** Skylink BTP Project Team

---

## 1. Executive Summary: Are You Doing It Right?

**Yes! You are doing it 100% right.** The combination of the **Pixhawk 2.4.8 Autopilot Kit** and the **F450 Quadcopter Frame Kit** is the absolute industry standard for custom research, companion computer prototyping, and autonomous flight testing. 

*   **F450 Propulsion Match:** The A2212 1000KV brushless motors paired with 30A ESCs and 1045 propellers provide the high-torque, high-efficiency lift needed to carry your flight controller, high-capacity battery, ESP32 companion computer, and 4G cellular modem.
*   **Pixhawk 2.4.8 Capability:** The Pixhawk 2.4.8 runs the stable ArduPilot (ArduCopter) stack and provides redundant processing, advanced sensor filtering (EKF3), and dedicated serial ports (`TELEM1`, `TELEM2`) for high-speed MAVLink telemetry routing.
*   **The Critical Substitutions (Robokits Kit):**
    > [!WARNING]
    > **The MS5607 Barometer Substitution:** The Robokits kit (RKI-7227) uses the MEAS MS5607 barometer instead of the standard MS5611. ArduPilot will calculate incorrect altitudes out of the box, leading to rapid altitude drift or pre-arm failures. 
    >
    > **The Fix:** In Mission Planner, you **must** set **`BARO_OPTIONS = 1`** to force the MS5607 scaling math, and save before flight.

---

## 2. Brainstorming Internet-Only & GCS-Controlled Flight

You mentioned: **"we will be controlling the drone only via the internet."** (GCS-only control / Beyond Visual Line of Sight - BVLOS). 

While highly advanced and completely supported by ArduPilot, flying a drone over the internet (cellular 4G LTE/WiFi) without a physical RC transmitter introduces unique safety, regulatory, and electrical requirements.

```
  ┌──────────────────────┐             ┌─────────────────────┐             ┌─────────────────────┐
  │     Browser GCS      │   4G/WiFi   │    7Semi EC200U     │    UART     │    ESP32 DevKit     │
  │  (Internet Command)  ├────────────►│  Industrial Modem   ├────────────►│ (Skylink Companion) │
  │                      │   Socket    │   (Cellular Link)   │  Commands   │  (MAVLink Bridge)   │
  └──────────────────────┘             └─────────────────────┘             └──────────┬──────────┘
                                                                                      │
                                                                                   MAVLink
                                                                                      │
                                                                                      ▼
  ┌──────────────────────┐             ┌─────────────────────┐             ┌─────────────────────┐
  │  Safe Landing Area   │   Land /    │     Neo-M8N GPS     │    UART     │    Pixhawk 2.4.8    │
  │     (RTL Failsafe)   │◄────────────┤  (Position Source)  │◄────────────┤  (Flight Controller)│
  └──────────────────────┘   Return    └─────────────────────┘             └─────────────────────┘
```

### 🛑 Critical Risks of Internet-Only Control
1. **Link Latency & Dropped Connection:** Cellular connections can experience high latency spikes or complete dropouts. If your internet link disconnects, you will lose live camera feeds and telemetry command capabilities.
2. **No Manual Emergency Save:** If the ESP32 code hangs, or the cellular link sags, you have **no mechanical stick override** to save the drone from drifting away or crashing.
3. **Pre-Arm Safety Interlocks:** By default, ArduPilot will **refuse to arm** the motors if it does not detect a physical RC transmitter signal, as a safety precaution.

### 🛠️ The Safety Action Plan & Failsafe Parameters
To fly safely and bypass RC controller requirements, you must configure ArduPilot with robust GCS-only parameters. In Mission Planner, navigate to **Config** ──► **Full Parameter List** and apply the following settings:

| Parameter | Recommended Value | Detailed Purpose |
| :--- | :--- | :--- |
| **`FS_THR_ENABLE`** | **0** (Disabled) | Prevents the Pixhawk from locking up or triggering an immediate crash failsafe due to the absence of physical RC receiver signals. |
| **`FS_GCS_ENABLE`** | **1** (RTL) | **MANDATORY:** If the ESP32 cellular internet link drops for more than 5 seconds, the drone will automatically trigger **Return-to-Launch (RTL)**, climb to a safe altitude, fly back to its takeoff coordinates, and land autonomously. |
| **`ARMING_CHECK`** | **Exclude RC** (or set to **0** for bench tests) | Tells the autopilot's pre-arm safety checks to ignore the missing RC transmitter and receiver link. |
| **`SERIAL2_PROTOCOL`**| **2** (MAVLink 2) | Configures the `TELEM2` port on the Pixhawk for high-speed bi-directional telemetry transmission. |
| **`SERIAL2_BAUD`** | **115** (115200 bps) | Sets the serial connection speed to match the ESP32 UART speed. |

> [!CAUTION]
> **Safety Recommendation:** For your **initial bench testing and low-altitude hovering (Phase H1–H4)**, we highly recommend purchasing a very cheap, entry-level physical RC controller (such as the **FlySky FS-i6X with FS-iA6B receiver**, which costs ~₹3,500 – ₹4,000 INR). 
> 
> Having a physical radio switch to immediately trigger manual `STABILIZE` or `RTL` will save your prototype and prevent injury if an autonomous internet command lags or behaves unexpectedly.

---

## 3. Integrating Components You Already Own

You have a fantastic set of prototyping electronics. Here is exactly how we will integrate them into the Skylink F450 quadcopter:

### 1. ESP32 Development Board (30-Pin)
*   **System Role:** This is your **Skylink Companion Computer**. It runs the embedded web server, serves the map-first Ground Control Station (GCS) dashboard, establishes the 4G/WiFi internet socket, and bridges GCS WebSocket commands into serial MAVLink packets for the Pixhawk.
*   **Integration:** Mounted directly onto the center plate. Connects to the Pixhawk’s `TELEM2` port via Hardware Serial 2 (GPIO16/17).

### 2. XL4015 Buck Converter Module (5A Rated)
*   **System Role:** This is your **Isolated Companion Power Supply**. 
*   **Integration:** Tap your drone's high-current battery distribution board (3S/4S LiPo) and wire the input of the XL4015. Adjust the multi-turn potentiometer on the XL4015 until the output reads **exactly 5.0V DC**. Wire this stable 5V output to the `VIN` and `GND` pins of your ESP32 board.
*   **Why this is perfect:** Cellular modems and ESP32 WiFi transmission bursts draw sudden current spikes. The XL4015 provides a dedicated, heavy-duty 5A current path, keeping your companion electronics powered and **completely isolated** from the Pixhawk's internal 5V rail, preventing critical flight-controller brownouts.

### 3. Solderless Breadboard, DuPont Jumper Wires & Perfboards
*   **System Role:** Prototyping & Bench Testing.
*   **Integration:** Use the breadboard and jumper wires to wire the ESP32, Pixhawk, and 4G modem together on your bench first. Once you have verified the internet telemetry links, use the **Perfboards** to solder the final, vibration-resistant wiring shield that will mount onto the F450 frame.

---

## 4. The Complete Bill of Materials (BOM)

To bring your Skylink F450 drone from a box of parts to a fully functional, internet-controlled aircraft, you must order the following remaining components:

### A. Core Cellular Telemetry & Location Link
| Component | Estimated Price (INR) | Sourcing URL / Vendor | Purpose |
| :--- | :--- | :--- | :--- |
| **7Semi EC200U LTE 4G GNSS Mini Industrial Modem (26V Range)** | **₹1,769.00** (incl. GST) | [Robodo.in Product Page](https://robodo.in/products/7semi-ec200u-lte-4g-gnss-mini-industrial-modem-usb-c-26v-range) | Provides the 4G cellular internet link for unlimited range telemetry. The "26V Range" allows it to be powered directly from the LiPo battery. |
| **Active GPS Antenna (u.FL/IPEX connector)** | **₹300.00** | [Robodo / Robocraze / Local](https://robodo.in/products/active-gps-antenna-with-ipex-connector) | Required to activate the GNSS location tracking feature on the cellular modem for dual-redundant tracking. |
| **Active 4G Nano-SIM Card** | **₹100.00 + Plan** | Jio, Airtel, or Vodafone VI | Plugs into the modem's Nano-SIM slot to provide network data access. |

### B. High-Current Power & Propulsion
| Component | Estimated Price (INR) | Sourcing URL / Vendor | Purpose |
| :--- | :--- | :--- | :--- |
| **Orange 3S 3300mAh 30C/35C LiPo Battery** (or 2200mAh to 4200mAh) | **₹1,800.00 – ₹2,500.00** | [Robokits India](https://robokits.co.in/batteries-chargers/lipo-battery-standard) / Local | The main power source for the brushless motors, ESCs, Pixhawk, ESP32, and 4G modem. |
| **IMAX B6AC V2 Balance Charger** (or SkyRC e3/e4) | **₹1,500.00 – ₹3,500.00** | Robokits / Robocraze / Local | **MANDATORY:** Safely charges and balances the individual cells of your LiPo batteries. |
| **XT60 Male Pigtail Connector (12 AWG/14 AWG)** | **₹80.00** | Robokits / Robocraze / Local | Plugs the Power Module's high-current output line directly into the F450 PDB center plates. |

### C. Assembly & Solder Essentials
| Component | Estimated Price (INR) | Sourcing / Vendor | Purpose |
| :--- | :--- | :--- | :--- |
| **Soldering Iron (60W or higher)** | **₹450.00 – ₹1,200.00** | Local Electronics / Robocraze | Required for tinning the massive copper power distribution planes on the F450 fiberglass plate. |
| **Solder Wire (60/40 Rosin Core) & Flux** | **₹150.00** | Local Electronics | For making low-resistance high-current electrical connections. |
| **Heat Shrink Tubing (2mm, 4mm, 6mm)** | **₹100.00** | Local Electronics | For insulating motor-to-ESC and power joint solder connections on the arms. |
| **Zip Ties & Double-Sided Foam Tape** | **₹150.00** | Local Hardware | Securely mounts the ESCs under the arms and fastens the Pixhawk shock-absorber and ESP32. |

---

## 5. Wiring & Power Distribution Architecture

Once all parts are secured, assemble and wire your drone according to the following layout. This ensures **complete power isolation** between the propulsion, control, and companion telemetric blocks.

```
                               ┌────────────────────────────────┐
                               │   3S/4S LiPo Battery (11.1V)   │
                               └───────────────┬────────────────┘
                                               │ (XT60 Power Connection)
                                               ▼
                               ┌────────────────────────────────┐
                               │ Pixhawk Power Module (Current) │
                               └───────┬────────────────┬───────┘
                                       │ (5.3V Clean)   │ (Raw Battery Power)
                                       ▼                ▼
  ┌──────────────────────┐     ┌───────────────┐┌───────────────┐     ┌──────────────────────┐
  │     Neo-M8N GPS      │◄───►│ Pixhawk 2.4.8 ││  F450 Center  │────►│ SimonK 30A ESCs (x4) │
  │  (Autopilot GPS/Com) │     │  (Autopilot)  ││   Plate PDB   │     │ (Power to Motors)    │
  └──────────────────────┘     └───────┬───────┘└───────┬───────┘     └──────────────────────┘
                                       │                │
                                    TELEM2           Battery
                                    MAVLink          Positive
                                     UART             (Raw)
                                       │                │
                                       ▼                ▼
                               ┌───────────────┐┌───────────────┐     ┌──────────────────────┐
                               │ ESP32 DevKit  ││ 7Semi EC200U  │◄───►│ High-Gain 8dBi       │
                               │   (Skylink)   ││ 26V Range 4G  │     │ FPC Cellular Antenna │
                               └───────▲───────┘└───────▲───────┘     └──────────────────────┘
                                       │                │
                                      5.0V             VCC
                                     Clean            Direct
                                       │                │
                               ┌───────┴───────┐        │
                               │ XL4015 Buck   │────────┘
                               │   Converter   │
                               └────────────────┘
```

### 🔋 Power Routing Rules
1. **Autopilot Power:** The **Pixhawk Power Module** connects directly to the battery and outputs a regulated **5.3V** directly to the Pixhawk `POWER` port.
2. **Propulsion Power:** The high-current output of the Power Module plugs into the **F450 Center Plate PDB**. The four **30A SimonK ESCs** are soldered directly to this PDB.
3. **Companion Power (ESP32):** Solder the input of the **XL4015 Buck Converter** to the F450 Center Plate PDB. Regulate the output to **5.0V** and connect it to the ESP32 `VIN` and `GND` pins.
4. **Modem Power:** Wire the **7Semi 4G Modem's VIN** directly to the F450 Center Plate PDB. Because it has a **26V wide-voltage range**, it will safely regulate the battery voltage down internally, isolating the radio's cellular transmit bursts from the companion processor.

---

### Associated Technical References:
* **Detailed 7Semi 4G Industrial Modem Report:** [7semi_ec200u_mini_industrial_modem.md](file:///d:/btp_skylink/Skylink/docs/components/networking/7semi_ec200u_mini_industrial_modem.md)
* **Robokits Pixhawk 2.4.8 Combo Kit Reference:** [pixhawk_248_robokits_combo.md](file:///d:/btp_skylink/Skylink/docs/components/kits/pixhawk_248_robokits_combo.md)
* **F450 Quadcopter Frame & Propulsion Reference:** [f450_quadcopter_frame_kit.md](file:///d:/btp_skylink/Skylink/docs/components/kits/f450_quadcopter_frame_kit.md)
* **Pixhawk 2.4.8 Global Hardware Roadmap:** [PIXHAWK_HARDWARE_ROADMAP.md](file:///d:/btp_skylink/Skylink/docs/PIXHAWK_HARDWARE_ROADMAP.md)
