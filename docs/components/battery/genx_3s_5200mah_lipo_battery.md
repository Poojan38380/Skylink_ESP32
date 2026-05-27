# GenX 11.1V 3S 5200mAh 40C/80C Premium LiPo Battery — Component Report

> [!NOTE]
> This reference document details the technical specifications, physical characteristics, performance calculations, and safety protocols for the **GenX Power Premium 11.1V 3S 5200mAh 40C / 80C LiPo Battery** (Model: RKI-4891) in the context of the **Skylink** F450 autonomous companion drone.
>
> **Product URL:** [Robokits GenX 11.1V 3S 5200mAh 40C Premium LiPo](https://robokits.co.in/batteries-chargers/drone-batteries/genx-power-premium-lipo-battery/genxpower-11.1v-lipo-batteries/genx-11.1v-3s-5200mah-40c-80c-premium-lipo-lithium-polymer-battery)
> **Retail Price:** ₹2,090.00 INR (Excl. GST) | **Total with 18% GST:** ₹2,466.20 INR
> **Model / SKU:** RKI-4891
> **Last Updated:** May 27, 2026

---

## 1. Overview & System Role

The **GenX 11.1V 3S 5200mAh 40C/80C Premium LiPo** is a high-energy-density lithium polymer battery pack engineered for professional and research-grade multirotors. For the **Skylink autonomous drone**, this battery serves as the **sole, heavy-duty energy reservoir** powering:
1.  **Propulsion Block:** Four high-current A2212 1000KV brushless motors via SimonK 30A ESCs.
2.  **Control Block:** The Pixhawk 2.4.8 Autopilot (regulated to 5.3V via the Power Module).
3.  **Companion Block:** The ESP32 DevKit WiFi MAVLink bridge (regulated to 5.0V via the XL4015 buck converter).
4.  **Network Block:** The 7Semi EC200U 4G LTE cellular modem (powered directly from the raw battery bus).

With its substantial **5200mAh capacity** and high **40C continuous discharge rate**, it provides the high current required during vertical takeoff maneuvers while sustaining stable voltage during long telemetry-guided hovering phases.

---

## 2. Technical Specifications

| Specification | Value | Engineering Significance |
| :--- | :--- | :--- |
| **Capacity** | **5200 mAh** (5.2 Ah) | Determines total electrical charge storage. |
| **Nominal Voltage** | **11.1 V** (3.7V per cell) | Standard nominal operating voltage for 3S systems. |
| **Fully Charged Voltage**| **12.6 V** (4.2V per cell) | The voltage threshold at 100% state-of-charge. |
| **Minimum Safe Voltage**| **10.5 V** (3.5V per cell) | **CRITICAL:** Low-voltage landing threshold to avoid cell damage. |
| **Configuration** | **3S1P** (3 Cells in Series) | 3 series-connected cells provide 11.1V nominal. |
| **Continuous Discharge** | **40C** (208 Amps) | Safe continuous current delivery capability ($5.2\text{A} \times 40 = 208\text{A}$). |
| **Burst Discharge Rate** | **80C** (416 Amps) | Safe peak current for short bursts ($5.2\text{A} \times 80 = 416\text{A}$). |
| **Net Weight ($\pm 2\%$)** | **414 grams** | Crucial variable in calculating total takeoff weight. |
| **Dimensions (LxWxH)** | **138mm × 43mm × 25mm** | Fits cleanly within the lower F450 frame tray. |
| **Discharge Connector** | **XT60 Male** | Plugs directly into the Pixhawk Power Module. |
| **Balance Connector** | **JST-XH 4-Pin** | Fits standard balance ports for cell balancing. |

---

## 3. Flight Performance & Endurance Math

To determine if this battery is a good match for the **Skylink F450 Quadcopter**, we must evaluate its weight contribution, thrust-to-weight ratio, and flight time limits.

### 3.1 Total Takeoff Weight (TOW) Calculation
Using the precise weight breakdown of the Skylink drone hardware:

$$\text{Empty Weight (F450 Frame + Motors + ESCs + Pixhawk + ESP32 + 4G Modem + Wires)} = 882\text{ grams}$$
$$\text{GenX 3S 5200mAh Battery Weight} = 414\text{ grams}$$
$$\mathbf{\text{Total Takeoff Weight (TOW)}} = 882\text{g} + 414\text{g} = \mathbf{1,296\text{ grams}} \text{ (~1.3 kg)}$$

### 3.2 Thrust-to-Weight Ratio
*   **Maximum Thrust:** A2212 1000KV motors with 1045 propellers on 3S voltage yield **~830g** of thrust per motor at 100% throttle.
*   **Total Max Thrust (4 Motors):** $4 \times 830\text{g} = 3,320\text{ grams}$
*   **Ratio:** 
    $$\text{Thrust-to-Weight Ratio} = \frac{3,320\text{g}}{1,296\text{g}} \approx \mathbf{2.56 : 1}$$

> [!TIP]
> A ratio of **2.56:1** exceeds the standard minimum safety guideline of **2.0:1** for multirotors. This ensures the drone has plenty of power to execute rapid altitude recovery and counter outdoor wind resistance. The drone will maintain a stable hover at **~39% throttle**.

### 3.3 Hover Current Draw
At hover, the brushless motors operate at a nominal efficiency of **6.5 grams of thrust per Watt (g/W)**:

1.  **Total Hover Power Required:**
    $$\text{Hover Power (W)} = \frac{\text{Takeoff Weight}}{\text{Efficiency}} = \frac{1,296\text{g}}{6.5\text{g/W}} \approx \mathbf{199.38\text{ Watts}}$$
2.  **Average Hover Current (at 11.1V nominal):**
    $$\text{Average Current (A)} = \frac{\text{Hover Power}}{\text{Voltage}} = \frac{199.38\text{ W}}{11.1\text{ V}} \approx \mathbf{17.96\text{ Amps}}$$

### 3.4 Flight Time Estimation (The 80% Rule)
Discharging a LiPo battery below 20% of its capacity damages the chemical cells and causes severe voltage sag. Therefore, we use **80% of the nominal capacity (4,160 mAh / 4.16 Ah)** as our usable limit:

$$\text{Hover Flight Time (Safe Limit)} = \frac{\text{Usable Capacity (Ah)} \times 60\text{ minutes}}{\text{Average Hover Current (A)}} = \frac{4.16\text{ Ah} \times 60}{17.96\text{ A}} \approx \mathbf{13.9\text{ minutes}}$$
$$\text{Hover Flight Time (Theoretical Max - 100\% DoD)} = \frac{5.2\text{ Ah} \times 60}{17.96\text{ A}} \approx \mathbf{17.4\text{ minutes}}$$

---

## 4. Hardware Wiring & Power Routing

The GenX battery integrates into the Skylink power distribution bus as shown in the schematic below:

```
                  ┌────────────────────────────────────────────────────────┐
                  │           GenX 3S 5200mAh LiPo Battery (11.1V)         │
                  └──────┬───────────────────────────────────┬─────────────┘
                         │ (XT60 Discharge Plug)             │ (JST-XH Balance Plug)
                         ▼                                   ▼
          ┌──────────────────────────────┐          ┌──────────────────────┐
          │ Pixhawk Power Module (XT60)  │          │ IMAX B6AC Charger    │
          └──────────────┬───────────────┘          │ (Cell-by-Cell Bal)   │
                         │                          └──────────────────────┘
                  ┌──────┴───────────────┐
                  │   F450 Center PDB    │
                  └──┬───┬───┬───┬───┬───┘
                     │   │   │   │   │
                    ESC ESC ESC ESC  ├───────────────► 7Semi EC200U 4G Modem
                    M1  M2  M3  M4   │                 VIN (Wide Input 5-26V)
                                     ▼
                            ┌────────────────┐
                            │  XL4015 Buck   │
                            │ (Steps down    │
                            │  voltage to    │
                            │  5.0V clean)   │
                            └────────┬───────┘
                                     ▼
                              ESP32 DevKit VIN
```

### 🔋 Crucial Wiring Guidelines:
1.  **Discharge Connection:** The XT60 male connector of the battery plugs directly into the female XT60 input of the **Pixhawk Power Module**. The output of the Power Module is soldered to the primary positive (`+`) and negative (`-`) copper traces of the **F450 PDB**.
2.  **Impedance-Matched Cells:** Each pack has matched internal resistance, minimizing heat generation during high-current operations.
3.  **Low Voltage Telemetry Alarm:** Because the battery is wired to the Pixhawk Power Module, real-time voltage and remaining capacity are broadcast over MAVLink. In the Skylink JavaScript dashboard (`data/gcs_config.js`), ensure `preflightMinBatteryPct` is configured to **20%** to trigger visual warnings in the GCS.

---

## 5. Storage & Safety Guide

LiPo batteries are chemically sensitive energy storage devices and require careful handling:

1.  **Charging Safety:** Always balance-charge the GenX battery using the **IMAX B6AC** charger at a safe **1C rate (5.2 Amps)**. Never charge a warm battery immediately after flight; allow it to cool down to ambient temperature.
2.  **Storage Voltage:** If the drone is going to sit idle for more than 48 hours, use the IMAX B6AC's **Storage Mode** to discharge/charge each cell to **3.8V – 3.85V** (approx. 11.5V total). Storing a LiPo fully charged (12.6V) or fully discharged (<10.5V) causes cell swelling, internal shorts, and dramatic capacity loss.
3.  **Physical Protection:** Store and charge the battery inside a fireproof **LiPo Safe Bag**. Keep it away from combustible materials.
4.  **No Heat:** Do not expose the pack to direct sunlight or temperatures above 60°C.

---

## 6. Verdict: Is this a good choice for Skylink?

> [!IMPORTANT]
> **YES! The GenX 3S 5200mAh LiPo battery is the ABSOLUTE BEST choice for the Skylink F450 companion drone.**
>
> **Why:**
> *   **Long Flight Endurance:** It provides a safe **14 minutes of stable hover**, which is more than double the 7-minute limit of a 2200mAh pack. This gives you plenty of time to establish internet networks, send MAVLink commands, and land safely.
> *   **High Performance-to-Weight Ratio:** At 414g, it is one of the most compact 5200mAh packs, keeping the thrust-to-weight ratio at a highly responsive 2.56:1.
> *   **Robust C-Rating:** The 40C continuous (208A) capacity ensures zero voltage sag under motor load.
> *   **Cost-Effective:** At ₹2,466 (incl. GST), it represents exceptional value for a high-end research battery.
