# Skylink ESP32 IoT Platform

A sophisticated ESP32 IoT application with web dashboard, OTA updates, and modular architecture.

## 🚀 Features

- **WiFi Manager**: Auto-reconnect with configurable credentials
- **Web Dashboard**: Responsive UI with real-time device monitoring
- **REST API**: JSON endpoints for device, WiFi, and system status
- **LED Control**: Remote control of built-in LED via web interface
- **OTA Updates**: Wireless firmware updates over WiFi
- **NTP Time Sync**: Automatic time synchronization
- **EEPROM Storage**: Persistent WiFi credentials and settings
- **Modular Architecture**: Clean separation of concerns

## 📋 Project Structure

```
Skylink/
├── include/               # Header files
│   ├── config.h          # Configuration constants
│   ├── logger.h          # Logging system
│   ├── wifi_manager.h    # WiFi connection manager
│   ├── web_server.h      # Web server module
│   ├── config_manager.h  # EEPROM configuration storage
│   ├── ota_updater.h     # OTA firmware updates
│   └── time_sync.h       # NTP time synchronization
├── src/                  # Source files
│   ├── main.cpp          # Application entry point
│   ├── config.cpp        # Configuration definitions
│   ├── logger.cpp        # Logger implementation
│   ├── wifi_manager.cpp  # WiFi manager implementation
│   ├── web_server.cpp    # Web server implementation
│   ├── config_manager.cpp # Config storage implementation
│   ├── ota_updater.cpp   # OTA updater implementation
│   └── time_sync.cpp     # Time sync implementation
├── data/                 # LittleFS (index.html, wifi_networks.json)
├── include/
│   └── flight_controller.h  # MAVLink bridge (SITL TCP or UART)
├── src/
│   └── flight_controller.cpp
├── platformio.ini        # -D SITL_MODE for simulation
└── docs/
    └── simulation/       # SITL run guides (read successful_run_guide.md)
```

## 🔧 Setup Instructions

### Prerequisites

- [PlatformIO](https://platformio.org/) installed (VS Code extension or CLI)
- ESP32 development board
- USB cable for initial flashing

### Configuration

1. **Edit WiFi Credentials** (optional - can be changed via web UI):
   
   Open `include/config.h` and modify:
   ```cpp
   const char* WIFI_SSID = "YOUR_WIFI_SSID";
   const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
   ```

2. **Build the Project**:
   ```bash
   pio run
   ```

3. **Upload to ESP32**:
   ```bash
   pio run --target upload
   ```

4. **Monitor Serial Output**:
   ```bash
   pio device monitor
   ```

## 🛰️ SITL Flight Simulation (ArduPilot + ESP32 GCS)

Skylink acts as a **WiFi MAVLink bridge** between a browser dashboard and ArduPilot SITL (or a physical Pixhawk in hardware mode).

**Start here:** [docs/simulation/successful_run_guide.md](docs/simulation/successful_run_guide.md)  
**AI / developer index:** [docs/simulation/README.md](docs/simulation/README.md)  
**Real drone (Pixhawk 2.4.8):** [docs/PIXHAWK_HARDWARE_ROADMAP.md](docs/PIXHAWK_HARDWARE_ROADMAP.md)

### Quick start (verified May 2026)

| Step | Where | Command / action |
|------|--------|------------------|
| 1 | WSL | `cd ~/ardupilot/ArduCopter && sim_vehicle.py -v ArduCopter` |
| 2 | Windows | `pio run --target uploadfs --target upload` |
| 3 | Mission Planner | TCP `127.0.0.1:5762` |
| 4 | Chrome | `http://<ESP32_IP>/` → wait for **MAVLink OK** |
| 5 | Dashboard | GUIDED → SET MODE → ARM → TAKEOFF |

**Ports:** MavProxy `5760` · Mission Planner `5762` · ESP32 `5763` — do not use `--out=tcpin:0.0.0.0:5763`.

---

## 🌐 Web Dashboard

Once connected to WiFi, access the dashboard at:
```
http://<ESP32_IP_ADDRESS>/
```

### Features:
- **Live MAVLink telemetry** from SITL or Pixhawk (altitude, GPS, battery, speed)
- **Flight control**: GUIDED mode, arm, autonomous takeoff, land, RTL
- **WiFi / link status** including SITL TCP connection state
- **LED demo** controls (bench testing)
- **Time Display**: NTP-synchronized clock

## 📡 REST API Endpoints

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/` | GET | Web dashboard (HTML) |
| `/config` | GET | Configuration page (HTML) |
| `/api/device` | GET | Device information (JSON) |
| `/api/wifi` | GET | WiFi status (JSON) |
| `/api/wifi/config?ssid=X&password=Y` | GET | Save WiFi credentials |
| `/api/system` | GET | System status (JSON) |
| `/api/led?state=true` | GET | Control LED state |

### Example API Responses:

**Device Info:**
```json
{
  "chip": "ESP32",
  "cpu_frequency_mhz": 240,
  "uptime_seconds": 12345,
  "free_heap_bytes": 280000,
  "flash_chip_size": 4194304,
  "sdk_version": "v4.4.3"
}
```

**WiFi Status:**
```json
{
  "connected": true,
  "ssid": "MyWiFi",
  "ip_address": "192.168.1.100",
  "signal_strength_dbm": -65,
  "mac_address": "AA:BB:CC:DD:EE:FF"
}
```

## 🔌 OTA (Over-The-Air) Updates

After initial USB flash, you can update firmware wirelessly:

1. **Using PlatformIO CLI**:
   ```bash
   pio run --target upload --upload-port <ESP32_IP_ADDRESS>
   ```

2. **Using VS Code**:
   - Click the PlatformIO icon in the sidebar
   - Go to Project Tasks → esp32dev → Upload to Remote Device

The OTA hostname is: `esp32-skylink`

## ⚙️ Configuration

### Timing Constants (config.h)

```cpp
WIFI_RECONNECT_INTERVAL = 10000    // WiFi reconnect attempt every 10s
HEARTBEAT_INTERVAL = 5000          // Heartbeat log every 5s
```

### EEPROM Storage

WiFi credentials and LED state are saved to ESP32's internal flash using the Preferences library. Data persists across restarts.

## 🔍 Serial Monitor Output

Example serial output:
```
[00:00:01] [INFO] ================================
[00:00:01] [INFO] Skylink ESP32 Starting
[00:00:01] [INFO] ================================
[00:00:01] [INFO] ConfigManager initialized
[00:00:02] [INFO] Connecting to WiFi: MyWiFi
[00:00:03] [INFO] WiFi connected! IP: 192.168.1.100
[00:00:03] [INFO] Setting up OTA updater: esp32-skylink
[00:00:03] [INFO] Configuring NTP time sync
[00:00:03] [INFO] Starting web server on port 80
[00:00:05] [INFO] Time synchronized: 14:30:45
[00:00:05] [INFO] Setup complete
[00:00:05] [INFO] Heartbeat | Time: 2026-04-14 14:30:45 | WiFi: MyWiFi | IP: 192.168.1.100 | Signal: -65 dBm
```

## 🐛 Troubleshooting

### WiFi Connection Issues
- **Problem**: ESP32 won't connect to WiFi
- **Solution**: 
  - Verify SSID and password are correct
  - Check router is broadcasting 2.4GHz (ESP32 doesn't support 5GHz)
  - Move ESP32 closer to router
  - Check serial monitor for error messages

### Web Dashboard Not Accessible
- **Problem**: Can't reach `http://<IP>/`
- **Solution**:
  - Verify ESP32 is connected to WiFi (check serial monitor)
  - Ensure your computer is on the same network
  - Check firewall settings
  - Try pinging the ESP32 IP address

### OTA Upload Fails
- **Problem**: OTA update doesn't work
- **Solution**:
  - Ensure ESP32 and computer are on same network
  - Check firewall isn't blocking mDNS
  - Try restarting ESP32
  - Use USB upload as fallback

### Time Sync Issues
- **Problem**: Time shows "Syncing..." or "N/A"
- **Solution**:
  - Verify WiFi is connected
  - Check NTP servers are accessible (default: pool.ntp.org)
  - Wait up to 2 minutes for initial sync
  - Router may be blocking NTP (port 123)

## 📊 Memory Usage

Current build (Phase 4):
- **RAM**: ~51KB / 327KB (15.5%)
- **Flash**: ~860KB / 1.3MB (65.6%)

## 🔄 Development Phases

This project was built in 5 phases:
1. **Foundation**: Modular architecture, WiFi manager, logger
2. **Web Server**: HTTP dashboard and REST API
3. **Interactive Controls**: LED control, WiFi config page, EEPROM storage
4. **Advanced Features**: OTA updates, NTP time sync
5. **Polish**: Documentation and error handling

## 📝 License

This project is open source. Feel free to use, modify, and distribute.

## 🤝 Contributing

Suggestions and improvements are welcome! This is a learning project - feedback helps everyone grow.

---

**Built with ❤️ using PlatformIO and ESP32**
