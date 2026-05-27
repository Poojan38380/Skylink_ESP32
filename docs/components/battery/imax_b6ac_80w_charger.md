# IMAX B6AC 80W AC/DC Balance Charger/Discharger — Component Report

> [!NOTE]
> This reference document details the technical specifications, charging efficiency calculations, operating modes, and safety guidelines for the **IMAX B6AC 80W AC/DC Balance Charger/Discharger (1-6 Cells)** (Model: RKI-1369). It evaluates the charger's compatibility and integration into the **Skylink** F450 companion drone project.
>
> **Product URL:** [Robokits IMAX B6AC 80W Charger/Discharger](https://robokits.co.in/batteries-chargers/chargers/imax-chargers/imax-b6ac-80w-charger-discharger-1-6-cells)
> **Retail Price:** ₹1,838.00 INR (Excl. GST) | **Total with 18% GST:** ₹2,168.84 INR
> **Model / SKU:** RKI-1369
> **Last Updated:** May 27, 2026

---

## 1. Overview & System Role

The **IMAX B6AC** is a high-performance, microprocessor-controlled battery charger, balancer, and discharger designed for a wide range of battery chemistries (LiPo, Li-ion, LiFe, NiMH, NiCd, and Pb). For the **Skylink autonomous drone project**, the IMAX B6AC serves as the **primary battery maintenance station**. 

Unlike standard balance chargers (such as the entry-level IMAX B3), the B6AC features:
1.  **Dual Input Power (AC/DC):** Integrated AC power supply allows plugging directly into any 100V–240V household wall outlet, eliminating the need for a separate, bulky external 12V DC power brick.
2.  **Adjustable Charge Rates:** Fully configurable charging current from 0.1A to 5.0A.
3.  **Active Storage Mode:** Safely charges or discharges LiPo batteries to storage voltage (3.85V per cell) to prevent swelling, capacity degradation, and fire hazards.
4.  **LCD Diagnostic Screen:** Real-time feedback on individual cell voltages, current input, elapsed charging time, and battery capacity (mAh).

---

## 2. Technical Specifications

| Specification | Value | Engineering Significance |
| :--- | :--- | :--- |
| **Input Voltage** | AC 100~240V | Direct wall plug charging convenience. |
| | DC 11.0~18.0V | Supports charging in the field from a car battery. |
| **Circuit Power (Max Charge)** | **50 W** (Standard) | The physical power limit for charging operations. |
| **Circuit Power (Max Discharge)**| **5 W** | The physical power limit for discharging operations. |
| **Charge Current Range** | **0.1 ~ 5.0 A** | Fully adjustable based on battery capacity. |
| **Discharge Current Range** | **0.1 ~ 1.0 A** | For battery health cycling and safe capacity tests. |
| **Li-ion/LiPo/LiFe Cell Count** | **1 ~ 6 Cells** in series | Supports everything from single-cell receivers to 6S packs. |
| **NiCd/NiMH Cell Count** | **1 ~ 15 Cells** | For standard AA/AAA cylindrical chemistry packs. |
| **Pb Battery Voltage** | **2 ~ 20 V** | For charging lead-acid field batteries. |
| **Net Weight** | **580 grams** | Solid, heavy-duty build with integrated aluminum heatsink. |
| **Dimensions** | **133mm × 87mm × 33mm** | Compact bench footprint. |

> [!WARNING]
> **Power Discrepancy Note:** While the product is marketed as **"IMAX B6AC 80W"** in the vendor's title, the detailed manufacturer specifications (RKI-1369) list the maximum charging circuit power as **50 Watts**. This 50W limit is standard for B6AC chargers and will govern the actual maximum charging current for higher-voltage batteries.

---

## 3. Battery Charge Time Calculations

To evaluate the efficiency of the IMAX B6AC, we must calculate the exact charging time for the **GenX 11.1V 3S 5200mAh LiPo Battery**.

### 3.1 Understanding the 50W Charger Power Limit
To balance-charge a 3S LiPo battery, the charger must supply power up to the battery's fully charged terminal voltage ($4.2\text{V} \times 3 = 12.6\text{V}$).

*   **Ideal 1C Charging Power:** Charging a 5200mAh (5.2Ah) battery at a standard **1C rate** requires:
    $$\text{Power Required} = \text{Voltage} \times \text{Current} = 12.6\text{ V} \times 5.2\text{ A} = \mathbf{65.52\text{ Watts}}$$
*   **The 50W Cap in Action:** Since the charger is physically capped at **50 Watts**, it will automatically throttle the charging current during the peak Constant Current (CC) phase to prevent overheating:
    $$\text{Maximum Real Charge Current} = \frac{\text{Power Cap}}{\text{Max Voltage}} = \frac{50\text{ W}}{12.6\text{ V}} \approx \mathbf{3.96\text{ Amps}}$$

Thus, even if you configure the charger to 5.0A on the screen, the microprocessor will automatically regulate the current down to **~3.96 Amps** to protect the internal circuitry.

---

### 3.2 Charging Time Estimation (Empty to 100%)
With a maximum real-world charge current of 3.96A:

1.  **Constant Current (CC) Phase:**
    $$\text{Charge Time (CC)} \approx \frac{\text{Battery Capacity (Ah)}}{\text{Actual Current (A)}} = \frac{5.2\text{ Ah}}{3.96\text{ A}} \approx 1.31\text{ hours} \approx \mathbf{78\text{ minutes}}$$
2.  **Constant Voltage (CV) Balancing Phase:** The balancer spends an additional 15–20 minutes at the end of the cycle to bleed off excess charge from individual cells, ensuring all 3 cells reach exactly 4.20V.
3.  **Total Charge Time:** **~1.5 hours (90 minutes)**.

> [!TIP]
> Compared to the basic IMAX B3 charger included with standard battery bundles (which takes **3.5+ hours**), the IMAX B6AC is **2.3 times faster**, turning a frustrating lab delay into a quick, manageable turnaround time.

---

## 4. Key Operating Modes & Program Configuration

For the Skylink F450 battery pack (3S 5200mAh LiPo), configure the charger settings as follows:

```
                            ┌─────────────────────────────────┐
                            │      IMAX B6AC PROGRAM SELECT   │
                            └────────────────┬────────────────┘
                                             │
                       Select: [PROGRAM SELECT: LiPo BATT]
                                             │
                        ┌────────────────────┴────────────────────┐
                        │                                         │
                        ▼ [Mode: LiPo CHARGE]                     ▼ [Mode: LiPo STORAGE]
              ┌──────────────────┐                      ┌──────────────────┐
              │ Current: 3.9A    │                      │ Current: 1.0A    │
              │ Voltage: 11.1V   │                      │ Voltage: 11.1V   │
              │ (3S balance plug)│                      │ (Brings cells to │
              └──────────────────┘                      │  3.85V safely)   │
                        │                               └──────────────────┘
                        ▼
              ┌──────────────────┐
              │ Mode: LiPo BAL   │
              │ (RECOMMENDED for │
              │ regular charging)│
              └──────────────────┘
```

### 1. LiPo BALANCE (Recommended for Normal Charging)
*   **Settings:** Set current to **3.9A** (or 5.0A, as the charger will auto-limit to 3.96A anyway), select **11.1V (3S)**.
*   **Why:** This mode reads each cell's voltage independently through the JST-XH balance plug and actively bleeds off high cells. **Always use this mode** for routine charging to ensure cell safety and prevent premature battery puffing.

### 2. LiPo STORAGE (Crucial for Battery Health)
*   **Settings:** Set current to **1.0A**, select **11.1V (3S)**.
*   **Why:** If your drone is not going to fly for more than 48 hours, run this program. It automatically charges or discharges the battery until every cell reaches exactly **3.85V** (approximately 11.55V total). This is the chemical sweet spot for lithium polymer storage, preventing chemical decay and gas build-up.

### 3. LiPo DISCHARGE (For Capacity Testing)
*   **Settings:** Set current to **1.0A**, select **9.0V (3S limit)**.
*   **Why:** Discharges the battery to a safe low-limit to calculate the actual capacity remaining in older packs. Never set the discharge cutoff below 3.0V per cell (9.0V total).

---

## 5. Safety Checklist & Troubleshooting Warnings

### 🚨 Crucial Safety Rules:
1.  **Correct Chemistry Selection:** Never charge a LiPo battery using the NiMH or NiCd program. LiPos require a Constant Current / Constant Voltage (CC/CV) charging curve. Charging on a NiMH pulse curve will cause the LiPo battery to swell and catch fire.
2.  **Safety Cutoff Timers:** Navigate to **User Set Program** and ensure the **Safety Timer** is enabled and set to **120 minutes**, and the **Capacity Cutoff** is enabled and set to **5500mAh**. This acts as a physical fail-safe if a battery cell fails to signal a full charge.
3.  **Attend the Charger:** Never leave a charging battery completely unattended. Perform all charges on a non-flammable surface (like tile or concrete) and keep the battery inside a fireproof **LiPo Safe Bag**.

---

## 6. Verdict: Is this a good choice for Skylink?

> [!IMPORTANT]
> **YES! The IMAX B6AC 80W AC/DC Balance Charger is an ABSOLUTELY ESSENTIAL investment for your BTP project.**
>
> **Why:**
> *   **Built-in AC Supply:** Eliminates external power supply clutter. You plug it straight into the lab wall.
> *   **Storage Program:** Saves you thousands of rupees by preventing your expensive 5200mAh battery from swelling during long periods of programming and desk work.
> *   **High-Speed Turnaround:** Restores your battery in 1.5 hours instead of 3.5+ hours.
> *   **Microprocessor Diagnostics:** The LCD allows you to verify that all cells are balanced within $\pm 0.01\text{V}$, ensuring a reliable power source before taking off autonomously.
