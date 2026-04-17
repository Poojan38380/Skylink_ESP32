# System Architecture

This diagram illustrates the high-level architecture of the Skylink ESP32 IoT Platform, showing the relationship between the hardware, the firmware modules, and the user interface.

```mermaid
graph TD
    subgraph "Ground Control Station (Browser)"
        UI[GCS Dashboard<br/>HTML/CSS/JS]
        WS_Client[WebSocket Client]
        UI <--> WS_Client
    end

    subgraph "ESP32 Device"
        subgraph "Firmware (C++/Arduino)"
            WS_Server[Async WebSocket Server]
            WebServer[Async Web Server]
            LED_Ctrl[LED Controller]
            WiFi_Mgr[WiFi Manager]
            Config_Mgr[Config Manager]
            NTP[NTP Time Sync]
        end

        subgraph "Hardware"
            LED[Built-in LED / GPIO]
            Flash[LittleFS / Flash Storage]
        end
    end

    %% Interactions
    WS_Client <== "JSON via WebSocket (/ws)" ==> WS_Server
    UI -- "HTTP GET /" --> WebServer
    WebServer -- "Serve index.html" --> Flash
    WS_Server --> LED_Ctrl
    LED_Ctrl --> LED
    WiFi_Mgr -- "Load Networks" --> Flash
    WiFi_Mgr -- "Connect" --> WiFi((WiFi Network))
    WS_Server -- "Read State" --> LED_Ctrl
    Config_Mgr <--> Flash
```

## Description

1. **GCS Dashboard**: A premium dark-themed web interface served directly from the ESP32.
2. **WebSocket Channel**: Provides real-time, bi-directional telemetry and command delivery.
3. **Firmware Modules**: Decoupled modules handling specific tasks (WiFi, Web, Hardware Control).
4. **LittleFS**: Used to store `index.html`, `wifi_networks.json`, and other static assets.
