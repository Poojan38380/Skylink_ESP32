# ESP32 PlatformIO Setup Guide (April 2026)

## Project Setup Status

### Current Configuration
Your project at `D:\btp_skylink\Skylink` has a **basic PlatformIO setup** already in place:

**`platformio.ini` contents:**
```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
```

### What's Done ✅
- PlatformIO project structure created
- `platformio.ini` configured with ESP32 Dev Module board
- Arduino framework selected
- Standard directory structure (`src/`, `lib/`, `include/`, `test/`) in place

### What's Missing ❌
1. **Source code**: `src/` directory is empty (needs `main.cpp`)
2. **USB Serial configuration**: Missing critical build flags for ESP32 USB CDC
3. **Monitor speed**: Not configured for serial monitoring
4. **Upload speed**: Not optimized

---

## Recommended Configuration Updates

### Enhanced `platformio.ini`
Add these settings to your `platformio.ini` for proper ESP32 USB support:

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino

; USB Serial configuration (critical for ESP32)
build_unflags = 
  -DARDUINO_USB_MODE=1

build_flags = 
  -DARDUINO_USB_CDC_ON_BOOT=1
  -DARDUINO_USB_MODE=0

; Serial monitor settings
monitor_speed = 115200
upload_speed = 921600
```

**Why these flags matter:**
- `ARDUINO_USB_CDC_ON_BOOT=1`: Routes `Serial.print()` to USB port instead of UART pins
- `ARDUINO_USB_MODE=0`: Prevents compilation conflicts on modern ESP32 variants
- `monitor_speed`: Must match `Serial.begin()` in your code

---

## PlatformIO Project Structure

```
Skylink/
├── src/              # Main application code (main.cpp required)
├── include/          # Header files (.h)
├── lib/              # Custom/local libraries
├── test/             # Unit tests
├── .pio/             # Auto-generated build files & dependencies
├── .vscode/          # VS Code workspace configs
├── platformio.ini    # Core project configuration
└── .gitignore        # Git ignore rules
```

---

## Development Workflow

### 1. Create Main Source File
Create `src/main.cpp` with Arduino framework structure:
```cpp
#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  pinMode(2, OUTPUT);  // Built-in LED on most ESP32 boards
  Serial.println("ESP32 Started");
}

void loop() {
  digitalWrite(2, HIGH);
  delay(1000);
  digitalWrite(2, LOW);
  delay(1000);
}
```

### 2. Build/Compile
- Click **✓ (Checkmark)** in bottom status bar, OR
- Run: `pio run`

### 3. Upload to ESP32
- Click **→ (Arrow)** in bottom status bar, OR
- Run: `pio run --target upload`

**If upload fails ("Failed to connect"):**
- Hold the **BOOT** button on ESP32 while clicking upload
- Verify correct COM port is detected
- Check USB serial driver installed (CP210x or CH340)

### 4. Serial Monitor
- Click **🔌 (Plug icon)** in status bar, OR
- Run: `pio device monitor`
- Shortcut: `Ctrl + Alt + M`
- **Press RESET button** on ESP32 after opening monitor to see startup messages

### 5. Build + Upload in One Step
- Run: `pio run --target upload --upload-port <COM_PORT>`

---

## Library Management

### Add Libraries via `platformio.ini`
```ini
lib_deps =
  ArduinoJson
  https://github.com/DFRobot/DFRobot_SHT3x.git
  knolleary/PubSubClient@^2.8
```

PlatformIO auto-downloads to `.pio/libdeps/` on next build.

### Install via CLI
```bash
pio lib install <library_name>
```

---

## Troubleshooting

### Common Issues

| Issue | Solution |
|-------|----------|
| **Upload fails: "Failed to connect"** | Hold BOOT button during upload; verify COM port |
| **No serial output** | Check baud rate matches `monitor_speed`; press RESET after opening monitor |
| **Garbled serial output** | Baud rate mismatch between `Serial.begin()` and `monitor_speed` |
| **Library not found** | Add to `lib_deps` in `platformio.ini`; run `pio lib install` |
| **First build takes forever** | Normal—toolchain downloads (~30+ min); subsequent builds are fast |

### USB Serial Drivers
- **Official ESP32 boards**: CP210x driver
- **Clone/budget boards**: CH340 driver
- Download from manufacturer websites

### Verify PlatformIO Installation
```bash
pio --version
```

---

## VS Code PlatformIO Extension Features

- **PlatformIO Home**: Click ant icon in sidebar
- **Project Tasks**: Access build/upload/monitor from status bar
- **Library Manager**: Browse and install libraries via UI
- **Serial Monitor**: Built-in terminal with auto-connect
- **Debugging**: Supports hardware debugging via ESP-prog (JTAG)

---

## Next Steps

1. Update `platformio.ini` with USB CDC flags (see above)
2. Create `src/main.cpp` with your firmware code
3. Connect ESP32 via USB
4. Build and upload using PlatformIO
5. Monitor serial output

---

## Resources

- **Official Docs**: https://docs.platformio.org/
- **PlatformIO Examples**: https://github.com/platformio/platformio-examples
- **ESP32 Tutorials**: https://docs.platformio.org/en/latest/tutorials/index.html
- **DFRobot Guide**: https://wiki.dfrobot.com/tutorial/22588
- **SunFounder Guide**: https://www.sunfounder.com/blogs/news/how-to-set-up-esp32-on-platformio-with-vs-code-complete-step-by-step-guide

---

*Research Date: April 13, 2026*
*ESP32 Connected: Yes (USB)*
*Project Location: D:\btp_skylink\Skylink*
