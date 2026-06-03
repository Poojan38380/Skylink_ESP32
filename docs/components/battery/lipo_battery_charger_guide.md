# Skylink Drone Program: LiPo Battery & Charger Operational Manual
**Author:** Skylink Engineering Team
**Current Date:** June 4, 2026
**Target Component Suite:** GenX 3S 5200mAh 40C/80C LiPo Battery & IMAX B6AC 80W (50W Cap) AC/DC Balance Charger

---

> [!WARNING]
> **CRITICAL SAFETY WARNING:** Lithium Polymer (LiPo) batteries are chemically volatile. Incorrect handling, charging, discharging, or storage can result in thermal runaway, chemical fires, toxic gas release, and complete destruction of property. 
> 
> **NEVER** leave a charging LiPo battery unattended. **ALWAYS** charge the battery on a non-flammable surface inside a certified fireproof **LiPo Safe Bag**.

---

## 1. Overview & System Compatibility

For the **Skylink autonomous companion drone**, the [GenX 3S 5200mAh LiPo](file:///d:/btp_skylink/Skylink/docs/components/battery/genx_3s_5200mah_lipo_battery.md) acts as the sole power source, delivering high current to the ESCs/motors and clean stepped-down power to the Pixhawk flight controller, ESP32 companion block, and the 7Semi EC200U 4G LTE cellular modem. 

The [IMAX B6AC 80W Charger/Discharger](file:///d:/btp_skylink/Skylink/docs/components/battery/imax_b6ac_80w_charger.md) serves as the primary maintenance station. Understanding the intersection of their specifications is critical to safe and efficient operation.

### 1.1 Specification Comparison & Mathematical Bottlenecks

| Feature | [GenX 3S LiPo Battery](file:///d:/btp_skylink/Skylink/docs/components/battery/genx_3s_5200mah_lipo_battery.md) | [IMAX B6AC 80W Charger](file:///d:/btp_skylink/Skylink/docs/components/battery/imax_b6ac_80w_charger.md) | Engineering Significance |
| :--- | :--- | :--- | :--- |
| **Nominal Voltage** | **11.1 V** (3.7V / cell) | N/A | Match charger settings to 11.1V (3S). |
| **Maximum Voltage** | **12.6 V** (4.2V / cell) | N/A | Target cutoff voltage. |
| **Capacity** | **5200 mAh** (5.2 Ah) | N/A | Governs charge rate and safety limits. |
| **Standard Charge Rate (1C)** | **5.2 A** (Continuous) | Max **5.0 A** (Screen Limit) | The battery *can* handle 5.2A, but the charger can select up to 5.0A. |
| **Maximum Power Circuit** | N/A | **50 W** (True charging limit) | **The Bottleneck:** The charger caps current dynamically to stay below 50W. |
| **Discharge Capacity** | 40C Continuous (208A) | Max **5 W** / **1.0 A** | Discharging via charger is highly power-constrained. |
| **Main Interface Connector** | XT60 (Male) | Banana Plugs (4mm Output) | Requires Banana-to-XT60 adapter cable. |
| **Balance Interface** | JST-XH (4-Pin, 3S) | JST-XH Multi-Port Slot | Plugs directly into the 3S balance slot on charger. |

### 1.2 The 50W Charger Power Limit Explained
While the charger is marketed as an "80W" model, the internal AC-to-DC circuit and charge controller are capped at a maximum of **50 Watts** of output power. 

To charge a 3S LiPo, the charger must drive the voltage up to **12.6V** (4.20V per cell).
* **Ideal 1C Charging:** $12.6\text{ V} \times 5.2\text{ A} = \mathbf{65.52\text{ W}}$ (exceeds the 50W limit).
* **Maximum Achievable Current:**
  $$\text{Max Current} = \frac{\text{Power Cap}}{\text{Max Voltage}} = \frac{50\text{ W}}{12.6\text{ V}} \approx \mathbf{3.96\text{ A}}$$

> [!TIP]
> **Recommended Charge Setting:** Program the charger to **3.9A** instead of forcing it to 5.0A. Forcing the charger to 5.0A causes the microprocessor to constantly throttle and run at its absolute thermal limits (100% duty cycle), generating excessive heat and shortening the charger's lifespan. Operating at **3.9A** keeps the charger stable and cool while charging the battery in approximately **85–90 minutes**.

---

## 2. Wiring & Connection Diagram

You must connect **both** the high-current XT60 main leads and the JST-XH balance leads to the charger. Charging without the balance leads connected disables cell monitoring, which can cause single cells to overcharge and explode.

```mermaid
graph TD
    WallOutlet["AC Wall Outlet (100V-240V)"] -->|AC Power Cord| Charger["IMAX B6AC Charger (50W Cap)"]
    
    subgraph Connections ["Rear & Side Terminals"]
        Charger -->|Red Banana Plug (+)| MainCablePos["Banana-to-XT60 Cable (Red)"]
        Charger -->|Black Banana Plug (-)| MainCableNeg["Banana-to-XT60 Cable (Black)"]
        Charger -->|3S Balance Port| BalanceCable["JST-XH 4-Pin Connector"]
    end
    
    MainCablePos -->|XT60 Male| BatXT60["XT60 Female Plug"]
    MainCableNeg -->|XT60 Male| BatXT60
    BalanceCable -->|JST-XH Female| BatBalance["4-Pin JST-XH Male Port"]
    
    subgraph Battery ["GenX 3S 5200mAh LiPo Pack"]
        BatXT60
        BatBalance
        Cell1["Cell 1 (3.7V - 4.2V)"]
        Cell2["Cell 2 (3.7V - 4.2V)"]
        Cell3["Cell 3 (3.7V - 4.2V)"]
    end

    style Battery fill:#1a1a2e,stroke:#e94560,stroke-width:2px;
    style Charger fill:#16213e,stroke:#0f3460,stroke-width:2px;
```

### Connection Sequence Checklist:
1. **Safety First:** Inspect the battery pack. Ensure there is no physical damage, wire fraying, or cell puffing. Place the battery inside the **LiPo Safe Bag**.
2. **Power up Charger:** Connect the AC power cable to the wall outlet and the charger. The LCD will light up and beep.
3. **Connect Mains to Charger:** Plug the red (+) and black (-) 4mm banana plugs into the corresponding red and black outputs on the right side of the charger.
4. **Connect Balance Lead:** Plug the battery's 4-pin JST-XH balance plug into the **3S slot** on the side of the charger. The plug is keyed and only fits in one direction. Do not force it.
5. **Connect XT60 Lead:** Plug the XT60 male connector of the charge lead into the XT60 female connector of the battery.

---

## 3. First-Time Charger Calibration (Safety Cutoffs)

Before charging your GenX battery for the first time, you must configure the global safety limits in the charger's system menu. This prevents accidental overcharging or runtime overruns if the charger fails to detect the termination peak.

```
[Main Menu] -> Press 'TYPE/STOP' -> Navigate to [USER SET PROGRAM ->] -> Press 'ENTER'
```

Configure the following settings by pressing `ENTER` (value flashes), adjusting with `+`/`-`, and confirming with `ENTER`:

1. **Safety Timer:**
   * **Setting:** `ON` / `120 Min`
   * **Rationale:** The maximum expected charge time at 3.9A is ~90 minutes. A 120-minute timer cut-off prevents runaway charges.
2. **Capacity Cut-Off:**
   * **Setting:** `ON` / `5500 mAh`
   * **Rationale:** The nominal capacity is 5200mAh. If the battery takes in more than 5500mAh, the charger will terminate immediately to prevent swelling.
3. **Temp Cut-Off (Requires optional thermal probe):**
   * **Setting:** `ON` / `45°C` (113°F)
   * **Rationale:** LiPo batteries should not heat up during charging. Any temperature above 45°C indicates an internal short.
4. **Key Beep & Buzzer:**
   * **Setting:** `ON`
   * **Rationale:** Provides vital audio notifications for errors, completion, and warning cutoffs.

---

## 4. Step-by-Step Programming Guide

Use the four main buttons: **BATT TYPE / STOP**, **DEC ( - )**, **INC ( + )**, and **START / ENTER** to configure operating modes.

```
       ┌───────────┐  ┌───────────┐  ┌───────────┐  ┌───────────┐
       │ BATT TYPE │  │    DEC    │  │    INC    │  │   START   │
       │   STOP    │  │     -     │  │     +     │  │   ENTER   │
       └───────────┘  └───────────┘  └───────────┘  └───────────┘
```

---

### Procedure A: LiPo Balance Charging (Standard Daily Use)
> [!IMPORTANT]
> **Always use BALANCE mode.** Never use regular "CHARGE" or "FAST CHG" mode, as they bypass cell-by-cell balance regulation, which leads to cell drift and reduced lifespan.

```
[PROGRAM SELECT: LiPo BATT]
        │ (Press ENTER)
        ▼
[LiPo BALANCE] -> (Press ENTER to make current flash)
        │ Set to: [3.9A] (Press ENTER to make voltage flash)
        ▼
[11.1V(3S)] -> (Press and hold ENTER for 3 seconds)
        │
        ▼
[BATTERY CHECK... -> R: 3SER  S: 3SER CONFIRM(ENTER)]
        │ (Verify R and S both read 3SER, then press ENTER)
        ▼
[LP3S  3.9A  11.85V] -> Active charging screen
```

#### Monitoring the Charge Cycle:
During the charge cycle, monitor the LCD display:
* **LP3S:** Indicates 3S LiPo program.
* **3.9A:** Active charge current (may drop during Constant Voltage phase).
* **11.85V:** Real-time combined pack voltage (climbs toward 12.60V).
* **BAL:** Active balance charge routine.
* **000:00:** Elapsed timer (MM:SS).
* **00000:** Total capacity (mAh) delivered back into the pack.

**Diagnostic View:** Press the **INC ( + )** button to view individual cell voltages. You will see a screen like:
```
4.18  4.18  4.17
0.00  0.00  0.00
```
Ensure the three cells remain balanced within **$\pm 0.02\text{V}$** of each other. If one cell stays significantly lower (e.g. 4.15V, 4.15V, 3.85V), terminate the charge immediately; that cell is damaged.

---

### Procedure B: LiPo Storage Mode (Crucial for Battery Health)
> [!IMPORTANT]
> If you are not flying the drone within the next **48 hours**, you must bring the battery to storage voltage (**3.85V per cell / 11.55V total**). Storing a LiPo fully charged (12.6V) or fully discharged (<10.5V) causes cell swelling, capacity degradation, and increases the risk of fire.

The Storage program will automatically charge the battery if it is below 11.55V, or discharge it if it is above 11.55V.

```
[PROGRAM SELECT: LiPo BATT]
        │ (Press ENTER)
        ▼
[LiPo STORAGE] -> (Press ENTER to flash settings)
        │ Set current to: [1.0A]
        │ Set voltage to: [11.1V(3S)]
        ▼
(Press and hold ENTER for 3 seconds)
        │
        ▼
[BATTERY CHECK... -> R: 3SER  S: 3SER CONFIRM(ENTER)]
        │ (Confirm cell count and press ENTER)
        ▼
[LP3S  1.0A  11.55V] -> Active storage cycling
```

*Note: The discharge circuit is capped at **5 Watts**. At 11.5V, the actual discharge current will auto-limit to $\approx 0.43\text{A}$ to avoid overheating, even if set to 1.0A on the screen. The process can take several hours if discharging from a full state-of-charge.*

---

### Procedure C: LiPo Discharging (Capacity Testing)
Use this program to measure the real-world health of the battery by performing a controlled discharge down to the safe minimum limit of **3.2V per cell (9.6V total)**.

```
[PROGRAM SELECT: LiPo BATT]
        │ (Press ENTER)
        ▼
[LiPo DISCHARGE] -> (Press ENTER to flash settings)
        │ Set current to: [1.0A] (Max discharge limit)
        │ Set voltage to: [9.6V(3S)] (3.2V/cell cutoff)
        ▼
(Press and hold ENTER for 3 seconds)
        │
        ▼
[BATTERY CHECK... -> R: 3SER  S: 3SER CONFIRM(ENTER)]
        │ (Confirm and press ENTER)
        ▼
[LP3S  0.43A  11.20V] -> Active discharge monitoring
```

*Record the final mAh reading when the cycle finishes. If the discharged capacity is less than **80% of nominal (4160 mAh)**, the battery is reaching the end of its operational lifespan.*

---

## 5. Pixhawk & Skylink GCS Telemetry Calibration

To ensure the drone lands safely before the battery is depleted, you must configure the software safety limits on both the flight controller and the companion web app dashboard.

### 5.1 GCS Alarm Configuration
In the Skylink Ground Control Station web dashboard configuration file [data/gcs_config.js](file:///d:/btp_skylink/Skylink/data/gcs_config.js#L19), the minimum preflight checklist voltage threshold is set:

```javascript
// Inside d:\btp_skylink\Skylink\data\gcs_config.js
preflightMinBatteryPct: 20, // 20% capacity warning (corresponds to ~11.2V unloaded)
```

Ensure this limit is active. When the battery level drops below 20%, the Skylink UI will sound an alert, and the preflight checklist will block arming commands.

### 5.2 Flight Controller Failsafes (Mission Planner Setup)
In Mission Planner (connected to the Pixhawk via the ESP32 MAVLink bridge), navigate to **Config/Tuning -> Standard Params -> Battery Failsafe**:

1. **Battery Monitor:** Set to `Analog Voltage and Current` (Sensor: Pixhawk Power Module).
2. **Low Battery Voltage Failsafe:** Set to **10.5V** (3.5V per cell).
   * **Action:** Set to `Return-to-Launch (RTL)` or `Land`. 
   * **Rationale:** Under motor load, the battery voltage will sag. 10.5V is the absolute lowest threshold before permanent chemical cell degradation occurs.
3. **Critical Battery Voltage Failsafe:** Set to **10.2V** (3.4V per cell).
   * **Action:** Set to `Land` (forces an immediate descent on the spot).

---

## 6. Maintenance & Troubleshooting

### 6.1 Common Charger Error Codes & Actions

| Error Displayed | Probable Cause | Corrective Action |
| :--- | :--- | :--- |
| `REVERSE POLARITY` | Main charge lead plugged into charger backward. | Correct the banana plug insertion (Red to Red, Black to Black). |
| `CONNECTION BREAK` | Balance plug or XT60 plug became disconnected during the cycle. | Inspect banana plug terminals, XT60 connector, and balance plug seating. Check for broken wires. |
| `CELL ERROR` | Voltage difference between cells exceeds safe limits or cell count mismatch. | Press **INC** to view cell voltages. If one cell reads `< 2.5V` or is missing, discard the pack safely. |
| `SHORT ERR` | A short circuit occurred on the output side. | Turn off power immediately. Inspect main banana leads and XT60 adapter for exposed solder joints. |
| `OVER TIME LIMIT` | Safety timer expired before battery was fully charged. | Increase the safety timer in settings (e.g. 150 min) or verify that charge current is set to 3.9A. |

### 6.2 Life Cycle Maintenance Log
To track the health of your GenX battery pack, keep a written log containing:
* **Cycle Count:** Track the number of charges.
* **Charge Capacity (mAh):** Log the amount of capacity added. A dropping number indicates loss of cell capacity.
* **Internal Resistance (IR):** Check cell resistance if your charger supports it. Healthy cells should have an IR of **$< 10\text{ m}\Omega$** each. An IR above **$20\text{ m}\Omega$** indicates a degraded pack that should be retired from flight operations.

---

## 7. Emergency Protocols & Disposal Guide

### 7.1 Handling Battery Swelling (Puffing)
If the battery swells, looks bloated, or feels soft/spongy:
1. **Stop Charging:** Disconnect AC power from the wall immediately. **Do not pull the DC plugs directly from a hot battery.**
2. **Isolate:** Using heat-resistant gloves, place the battery inside a LiPo Safe Bag and move it outdoors to a concrete surface or into a metal bucket filled with dry sand.
3. **Cooling Period:** Leave it isolated for at least 2 hours. Do not attempt to reuse or puncture the cells.

### 7.2 Fire Extinguishing
If a LiPo catches fire:
* **Do not use standard water** unless there is no other choice, as lithium reacts with water (releasing hydrogen).
* **Best Extinguishing Agents:** Dry sand, Class D fire extinguisher, or CO2 extinguisher.
* Cover the battery with dry sand to suffocate the flame.

### 7.3 Disposal Procedure
Never throw a LiPo battery into standard household or lab waste. To safely dispose of a degraded or damaged LiPo:
1. Place the battery in a fireproof container outdoors.
2. Prepare a plastic bucket with a **saltwater solution** (approx. 1/2 cup of table salt per gallon of warm water).
3. Submerge the battery completely in the saltwater bath. The saltwater is conductive and will slowly discharge the battery down to absolute 0.0V over **24 to 48 hours**.
4. Remove the battery, wrap the terminals in electrical tape, and take it to an authorized local e-waste recycling center.
