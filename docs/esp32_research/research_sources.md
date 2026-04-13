# ESP32 PlatformIO Research Sources

## Primary Sources (April 2026)

### 1. DFRobot ESP32 PlatformIO Tutorial
- **URL**: https://wiki.dfrobot.com/tutorial/22588
- **Published**: January 16, 2026
- **Key Topics**:
  - PlatformIO installation with Python setup
  - Project creation and structure
  - ESP32 USB CDC configuration (critical build flags)
  - Library management via `lib_deps`
  - Build, upload, and serial monitor workflow
  - Best practices and troubleshooting

### 2. SunFounder Complete Guide
- **URL**: https://www.sunfounder.com/blogs/news/how-to-set-up-esp32-on-platformio-with-vs-code-complete-step-by-step-guide
- **Published**: October 23, 2025 (Updated 2026)
- **Key Topics**:
  - Prerequisites and driver installation (CP210x, CH340)
  - PlatformIO IDE verification
  - First project creation steps
  - platformio.ini configuration options
  - Upload troubleshooting (BOOT button technique)
  - Library installation methods

### 3. Official PlatformIO Documentation
- **URL**: https://docs.platformio.org/en/latest/tutorials/index.html
- **Key Topics**:
  - Arduino framework tutorials
  - ESP-IDF framework tutorials
  - Debugging guides (JTAG via ESP-prog)
  - Unit testing
  - Project analysis

### 4. PlatformIO YouTube Tutorial (March 2026)
- **URL**: https://www.youtube.com/watch?v=jOFBUVy34rM
- **Published**: March 28, 2026
- **Title**: "VS Code + PlatformIO Setup Guide (2026) | ESP32 & Arduino"

### 5. Reddit Community Guide
- **URL**: https://www.reddit.com/r/arduino/comments/1ril6zy/finally_moved_my_esp32_workflow_to_vs_code/
- **Published**: March 2, 2026
- **Title**: "Finally moved my ESP32 workflow to VS Code + PlatformIO on my Mac. Here is a quick 2026 setup guide"

## Additional Resources

### Video Tutorials
- "How to Setup ESP32 in VS Code using PlatformIO" (February 2026)
  - https://www.youtube.com/watch?v=F3oPVm2n-fk
- "Platform IO setup and Blink sketch on an ESP32" (March 2025)
  - https://www.youtube.com/watch?v=FDFxWScKswU

### Community Projects
- **ESPHome**: Home automation platform (https://esphome.io/)
- **ESP32-Paxcounter**: Wi-Fi/BLE tracking
- **OpenMQTTGateway**: Multi-protocol MQTT gateway

### Books
- "Developing IoT Projects with ESP32" - covers PlatformIO with ESP-IDF

## USB Serial Drivers

### CP210x Driver
- **For**: Official ESP32 boards (DOIT, DevKit)
- **Download**: https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers

### CH340 Driver
- **For**: Clone/budget ESP32 boards
- **Download**: Search "CH340 driver download"

## Key Technical Notes from Research

1. **USB CDC is Critical**: ESP32 native USB requires `ARDUINO_USB_CDC_ON_BOOT=1` to route Serial to USB
2. **First Build Time**: Initial toolchain download takes 30+ minutes
3. **BOOT Button**: Essential for entering flash mode on some boards
4. **RESET After Monitor**: Must press RESET to see startup serial output
5. **Build Flags Matter**: `ARDUINO_USB_MODE` conflicts cause compilation errors

---

*Compiled: April 13, 2026*
*Project: Skylink (D:\btp_skylink\Skylink)*
