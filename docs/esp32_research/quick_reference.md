# PlatformIO Quick Reference for ESP32 Development

## Essential Commands

```bash
# Check PlatformIO version
pio --version

# Build project
pio run

# Upload to ESP32
pio run --target upload

# Build and upload
pio run -t upload

# Open serial monitor
pio device monitor

# List connected devices
pio device list

# Install library
pio lib install <library_name>

# Clean build files
pio run --target clean

# Show project info
pio project run

# Run tests
pio test
```

## VS Code Shortcuts

| Action | Shortcut | Status Bar Icon |
|--------|----------|-----------------|
| Build | `Ctrl + Alt + B` | ✓ Checkmark |
| Upload | `Ctrl + Alt + U` | → Arrow |
| Serial Monitor | `Ctrl + Alt + M` | 🔌 Plug |
| Clean | - | 🗑️ Trash |
| New Project | - | 🏠 Home → New Project |

## platformio.ini Quick Config

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino

; USB Configuration
build_unflags = -DARDUINO_USB_MODE=1
build_flags = 
  -DARDUINO_USB_CDC_ON_BOOT=1
  -DARDUINO_USB_MODE=0

; Speed Settings
monitor_speed = 115200
upload_speed = 921600

; Libraries
lib_deps = 
  ArduinoJson
  knolleary/PubSubClient
```

## Common Board Types

| Board Name | platformio.ini Value | Notes |
|------------|---------------------|-------|
| ESP32 DevKit V1 | `esp32dev` | Most common |
| NodeMCU-32S | `nodemcu-32s` | Popular dev board |
| ESP32-WROOM-32 | `esp32dev` | Compatible with DevKit |
| ESP32-S3 | `esp32-s3-devkitc-1` | Newer variant |
| ESP32-C3 | `esp32-c3-devkitm-1` | RISC-V variant |

## Framework Options

| Framework | Description | Best For |
|-----------|-------------|----------|
| **Arduino** | Familiar Arduino API | Beginners, quick prototypes |
| **ESP-IDF** | Espressif's native SDK | Production, advanced features |
| **ESP-IDF + Arduino** | Both frameworks combined | Complex projects needing Arduino libs |

## Pin Reference (ESP32 DevKit)

### Common Pins
- **GPIO 2**: Built-in LED (most boards)
- **GPIO 1**: TX0 (Serial)
- **GPIO 3**: RX0 (Serial)
- **GPIO 21**: SDA (I2C)
- **GPIO 22**: SCL (I2C)
- **GPIO 18**: SCK (SPI)
- **GPIO 19**: MISO (SPI)
- **GPIO 23**: MOSI (SPI)

### ADC Pins
- **ADC1**: GPIO 32-39
- **ADC2**: GPIO 0, 2, 4, 12-15, 25-27

### Touch Pins
- GPIO 0, 2, 4, 12-15, 27, 32-33

## Debug Tips

1. **Enable verbose upload**: Add `upload_flags = --verbose` to `platformio.ini`
2. **Check device detection**: `pio device list`
3. **Force flash mode**: Hold BOOT button during upload
4. **Monitor baud rate**: Must match `Serial.begin()` exactly
5. **View build output**: PlatformIO → Terminal

## File Locations

| Purpose | Location |
|---------|----------|
| Main code | `src/main.cpp` |
| Headers | `include/` |
| Custom libs | `lib/` |
| Dependencies | `.pio/libdeps/` |
| Build artifacts | `.pio/build/` |
| Config | `platformio.ini` |

---

*Quick Reference - April 2026*
