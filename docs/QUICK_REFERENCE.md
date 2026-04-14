# Quick Reference Guide

## First Time Setup (5 minutes)

1. **Connect ESP32** to computer via USB
2. **Edit WiFi credentials** in `include/config.h` (or use web UI later)
3. **Build & Upload**:
   ```bash
   pio run --target upload
   ```
4. **Open Serial Monitor** to see IP address:
   ```bash
   pio device monitor
   ```
5. **Access Dashboard**: Open browser to `http://<ESP32_IP>/`

## Common Tasks

### Change WiFi Credentials
- **Via Web**: Go to `/config` page → Enter new SSID/Password → Save
- **Via Code**: Edit `include/config.h` → Re-upload via USB

### Update Firmware
- **USB**: `pio run --target upload`
- **OTA**: `pio run --target upload --upload-port <ESP32_IP>`

### Check Device Status
- **Web Dashboard**: `http://<ESP32_IP>/`
- **API**: `http://<ESP32_IP>/api/device`
- **Serial Monitor**: `pio device monitor`

## Pin Reference

| Pin | Component | Notes |
|-----|-----------|-------|
| GPIO 2 | Built-in LED | Active HIGH on most boards |
| - | - | Ready for your sensors! |

## Next Steps

Now that the foundation is ready, you can:
- Add sensors (DHT, BMP, ultrasonic, etc.)
- Add actuators (relays, servos, motors)
- Integrate with MQTT/Home Assistant
- Add Bluetooth LE support
- Create custom web widgets

Happy coding! 🚀
