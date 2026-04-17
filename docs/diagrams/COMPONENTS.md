# Firmware Component Diagram

A breakdown of the modular C++ architecture used in the Skylink ESP32 firmware.

```mermaid
classDiagram
    class Main {
        +setup()
        +loop()
    }
    
    class WiFiManager {
        +begin()
        +handle()
        +isConnected()
        -scanAndConnect()
    }
    
    class WebServer {
        +begin()
        -setupHandlers()
        -onWebSocketEvent()
    }
    
    class LEDController {
        +init()
        +set(bool state)
        +toggle()
        +getState() bool
    }
    
    class ConfigManager {
        +load()
        +save()
    }
    
    class OTAManager {
        +begin()
        +handle()
    }

    Main --> WiFiManager : "Initializes"
    Main --> WebServer : "Starts"
    Main --> LEDController : "Updates"
    Main --> OTAManager : "Handles"
    
    WebServer --> LEDController : "Dispatches Commands"
    WiFiManager ..> ConfigManager : "Uses Credentials"
    WebServer ..> WiFiManager : "Checks IP/SSID"
```

## Module Responsibilities
- **Main**: Orchestrates boot sequence and periodic tasks.
- **WiFiManager**: Manages the life-cycle of the wireless connection.
- **WebServer**: Handles asynchronous HTTP requests and WebSocket frames.
- **LEDController**: Hardware abstraction layer for actuators.
- **ConfigManager**: Interface for non-volatile storage (LittleFS/Preferences).
