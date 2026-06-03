# Skylink Internet Connectivity — Alternative Architectures & Evaluation

> [!NOTE]
> This research report evaluates alternative methods for providing a stable internet connection to the **Skylink** drone companion system (ESP32 / Pixhawk). It compares direct onboard cellular modems, WiFi offloading via portable routers, Linux-based companion computer integration, and long-range RF ground bridges.
>
> **Research Date:** June 2, 2026  
> **Prepared for:** Skylink BTP Project Team

---

## 1. The Core Connectivity Challenge: CGNAT & Power

When deploying an autonomous UAV that connects to a browser Ground Control Station (GCS) over the internet, two main engineering hurdles must be solved:

1.  **Carrier CGNAT (Carrier-Grade NAT):** Mobile carriers (like Jio, Airtel) do not assign public IP addresses to SIM cards. They place devices behind a private firewall. Because of this, your GCS laptop cannot directly connect to the drone's cellular IP address. The drone must connect outbound to a secure cloud server/relay, or run a software-defined network (like ZeroTier or Tailscale) to establish a peer-to-peer tunnel.
2.  **Power Spikes & Noise:** Cellular modules draw brief transient current surges of **up to 2.0A** during transmission. If the power supply is shared with sensitive components, these spikes trigger companion computer restarts or flight controller brownouts.

To bypass these hurdles, we evaluate four distinct architectures below.

---

## 2. Option A: Onboard Mobile Hotspot (MiFi / Portable Travel Router)

Instead of the ESP32 commanding a cellular modem via AT commands, you mount a small, battery-powered 4G WiFi router (e.g., **JioFi M2S**, **TP-Link M7200**, or a premium **GL.iNet GL-E750 Mudi**) directly on the drone frame.

```
┌─────────────────┐   UART   ┌──────────────┐   WiFi   ┌────────────────┐   4G LTE   ┌──────────────┐
│  Pixhawk 2.4.8  │ ◄──────► │ ESP32 Bridge │ ◄──────► │  Onboard MiFi  │ ◄────────► │ Cellular Net │
│  (Autopilot)    │  MAVLink │  (Skylink)   │  Local   │ (e.g., JioFi)  │            │  (Internet)  │
└─────────────────┘          └──────────────┘          └────────────────┘            └──────────────┘
```

### Why Choose It (The Advantages)
*   **Zero Serial Programming:** The ESP32 utilizes its simple, built-in WiFi stack (`WiFi.h`) to connect to the router's local SSID. You do not need to write complex AT command parsing, PPP dial-up, or cellular recovery code.
*   **Total Power Isolation:** The MiFi has its own internal rechargeable battery. This completely isolates the high-current cellular transmit pulses from the drone's flight electronics, eliminating the risk of in-flight brownouts.
*   **Multi-Device Bridging:** If you add a companion camera (like an ESP32-Cam or IP camera) for live video feeds, both the telemetry bridge and the camera can share the same internet connection simultaneously.
*   **Hardware VPN Tunneling (GL.iNet Only):** Premium travel routers like the **GL.iNet Mudi (GL-E750)** run OpenWrt and support native **WireGuard / OpenVPN / Tailscale** configurations. The router itself maintains a secure tunnel to your GCS network, bypassing carrier firewalls without any configuration on the ESP32.

### The Trade-offs
*   **Weight & Bulk:** A JioFi adds ~75g, while a GL.iNet Mudi adds ~150g to the aircraft. This reduces total flight time on an F450 frame.
*   **Separate Battery Maintenance:** You must remember to charge the MiFi battery before each flight session, or wire a separate USB charging line from a 5V/2A BEC.

---

## 3. Option B: Linux Companion Computer + USB Dongle (Industry Standard)

You replace the ESP32 companion computer with a single-board computer (SBC) like a **Raspberry Pi 4/5** or **NVIDIA Jetson**, and plug a standard **Huawei E3372 4G USB Dongle** directly into the Pi.

```
┌─────────────────┐   UART   ┌──────────────────┐   USB   ┌────────────────┐   4G LTE   ┌──────────────┐
│  Pixhawk 2.4.8  │ ◄──────► │   Raspberry Pi   │ ◄─────► │  Huawei E3372  │ ◄────────► │ Cellular Net │
│  (Autopilot)    │  MAVLink │ (Companion Comp) │         │ (4G LTE Dongle)│            │  (Internet)  │
└─────────────────┘          └──────────────────┘         └────────────────┘            └──────────────┘
```

### Why Choose It (The Advantages)
*   **Native OS Networking:** Linux has built-in drivers for USB cellular dongles (enumerated as standard Ethernet-over-USB interfaces). The OS handles connection dialing, tower handshakes, and automatic reconnect logic natively.
*   **VPN Overlay Support:** You can run standard networking clients like **Tailscale**, **ZeroTier**, or a private **WireGuard** profile directly on the Pi. This gives your drone a static, private IP address that your GCS laptop can connect to from anywhere in the world.
*   **HD Video Streaming:** The Raspberry Pi's processor can easily handle live H.264 video encoding. You can stream a 1080p drone camera feed over the 4G network alongside the telemetry stream using standard pipelines (GStreamer / WebRTC).
*   **Robust Telemetry Software:** Instead of custom C++ forwarding code, you can use robust, field-tested routing software like `mavlink-router` or `MAVProxy`.

### The Trade-offs
*   **High Power Consumption:** A Raspberry Pi 4/5 draws ~1.5A to 2.5A continuous at 5V, which requires a heavy-duty, clean BEC.
*   **Cost:** A Raspberry Pi 4/5 combo + USB LTE dongle costs ~₹10,000 to ₹12,000 INR (compared to ₹400 for an ESP32).

---

## 4. Option C: 900MHz RF-to-Internet Ground Bridge (BVLOS Safety Hybrid)

The drone does not carry a cellular SIM card or modem. Instead, the Pixhawk communicates via a long-range 900MHz RF telemetry transceiver (e.g., **RFD900x** or **SiK 900MHz Modem**) to a ground station receiver. The ground receiver is wired to your GCS laptop or a local Raspberry Pi/ESP32, which is connected to the internet (via home Wi-Fi or a local cellular hotspot) and relays the telemetry to a cloud GCS dashboard.

```
  [ ON THE DRONE ]                          [ ON THE GROUND ]
┌─────────────────┐   UART   ┌──────────────┐      RF Link     ┌──────────────┐   USB/UART   ┌──────────────┐   Internet   ┌─────────────┐
│  Pixhawk 2.4.8  │ ◄──────► │   RFD900x    │ ◄───(900MHz)───► │   RFD900x    │ ◄──────────► │  Ground PC / │ ◄──────────► │  Cloud GCS  │
│  (Autopilot)    │  MAVLink │  (Transceiver)│    Up to 40km    │ (Receiver)   │              │  ESP32 Relay │              │  Dashboard  │
└─────────────────┘          └──────────────┘                  └──────────────┘              └──────────────┘              └─────────────┘
```

### Why Choose It (The Advantages)
*   **Zero Cell Tower Dependency:** Cell tower coverage can drop or be highly congested in rural fields or at higher altitudes. A 900MHz RF link creates a direct point-to-point connection up to **40km** that works completely independently of public networks.
*   **No Cellular Data Costs:** No monthly SIM card data plans are required on the drone.
*   **Extremely Lightweight:** The RFD900x module weighs only ~15g and draws minimal power, avoiding thermal and battery sag problems on the aircraft.
*   **Ultimate Safety Failsafe:** Meets most BVLOS regulatory requirements since you maintain a direct, dedicated radio connection to the aircraft at all times.

### The Trade-offs
*   **Line of Sight Limits:** You must mount the ground antenna on a tripod or mast to clear trees, buildings, and terrain.
*   **Physical Hardware Cost:** A pair of RFD900x telemetry modems costs ~₹12,000 to ₹15,000 INR.

---

## 5. Architectural Comparison Matrix

| Factor | Direct Onboard Modem (e.g., 7Semi EC200U) | Onboard MiFi / Hotspot (e.g., JioFi / GL.iNet) | Companion Pi + Dongle | RF Ground Bridge (e.g., RFD900x) |
| :--- | :--- | :--- | :--- | :--- |
| **System Complexity** | **High:** Requires custom ESP32 AT command / PPP firmware logic. | **Low:** ESP32 treats it as standard Wi-Fi router. | **Medium:** Script routing on Linux; plug-and-play USB modem. | **Low:** Native ArduPilot telemetry routing. |
| **Hardware Cost** | **Lowest** (~₹1,769) | **Low to Medium** (~₹2,000 – ₹8,000) | **High** (~₹11,000) | **High** (~₹14,000) |
| **Onboard Weight** | **Lowest** (~25g with antenna) | **Medium** (~75g to 150g) | **High** (~100g with Pi, cables, & dongle) | **Lowest** (~15g with antenna) |
| **Power Stability** | **Poor:** Cellular pulses can sag ESP32 rail. Needs heavy BEC. | **Excellent:** Isolated internal battery prevents brownouts. | **Moderate:** High constant draw. Requires dedicated heavy-duty BEC. | **Excellent:** Low current draw (~100-800mA peak). |
| **VPN / CGNAT Bypass**| **Complex:** Requires cloud socket relay server. | **Easy (if GL.iNet):** Router runs native WireGuard client. | **Easy:** Single-line Tailscale or ZeroTier installation. | **Easy:** Handled by the internet-connected Ground PC. |
| **HD Video Support** | ❌ No (Cat 1 is too slow) | ❌ No (ESP32 CPU limit) | **✅ Yes** (Pi GPU handles encoding) | ❌ No (RF telemetry bandwidth only) |
| **Best Suited For** | Budget, telemetry-only custom coding project. | Quick setup, safe isolated power, and low software configuration. | Commercial-grade BVLOS, HD camera streaming, and AI flight tasks. | High-reliability flights in areas with poor or congested cell service. |

---

## 6. Summary Recommendations for Skylink

*   **Recommendation 1 (The Quickest & Safest Upgrade):**
    If you want to move the current Skylink code to a real drone with **minimal code changes** and **maximum electrical safety**, buy a portable 4G MiFi router (like a JioFi or a GL.iNet Beryl/Mudi) and mount it on the drone. Connect the ESP32 via Wi-Fi. This completely isolates your power rail, bypasses cellular AT coding, and allows you to test the SITL code on silicon immediately.
*   **Recommendation 2 (The Ultimate High-End Setup):**
    If this is for a high-end thesis project requiring **live video streaming** alongside telemetry, upgrade the ESP32 companion to a **Raspberry Pi 4/5** paired with a **Huawei E3372 4G Dongle**, and configure a **Tailscale / ZeroTier** overlay network.
*   **Recommendation 3 (The Rural / Range Setup):**
    If you intend to test your drone in open agricultural areas or forestry zones where cell reception is weak or non-existent, invest in a pair of **RFD900x modems** to bridge data to a ground laptop, and let the ground laptop push the telemetry to the internet.
