# Serial Communication — ESP32 to Autopilot

## Hardware Mode (UART2)
- **TX Pin:** GPIO 17
- **RX Pin:** GPIO 16
- **Baud Rate:** 115200
- **Config:** SERIAL_8N1 (8 data bits, no parity, 1 stop bit)

### Wiring
```
ESP32 GPIO17 (TX) ──→ Pixhawk TELEM2 RX
ESP32 GPIO16 (RX) ←── Pixhawk TELEM2 TX
ESP32 GND ──────────── Pixhawk GND
```
- Do NOT connect 5V from Pixhawk to ESP32 (voltage mismatch)

## SITL Mode (TCP)
- **Port:** 5763 (MAVProxy output)
- **Timeout:** 3000ms connect, 5000ms MAVLink activity
- **Command:** `--master tcp:127.0.0.1:5760 --out tcpin:0.0.0.0:5763`

## Baud Rate Selection
115200 is ArduPilot default for TELEM2. 921600 tested but caused buffer overruns on ESP32.
