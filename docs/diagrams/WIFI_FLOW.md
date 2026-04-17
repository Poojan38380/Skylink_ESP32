# WiFi Connection Flow

The Skylink platform uses a "Scan-and-Match" logic to identify and connect to the best available WiFi network without hardcoding credentials in the source code.

```mermaid
flowchart TD
    Start([Boot / Restart]) --> InitFS[Initialize LittleFS]
    InitFS --> LoadJSON[Load wifi_networks.json]
    LoadJSON --> Scan[WiFi.scanNetworks]
    
    Scan --> Sort[Sort Scan Results by RSSI]
    Sort --> Match{Match Found In<br/>Saved Networks?}
    
    Match -- No --> Wait[Wait 10s]
    Wait --> Scan
    
    Match -- Yes --> HighPriority[Select Highest Priority Match]
    HighPriority --> Connect[WiFi.begin]
    
    Connect --> Success{Connected?}
    
    Success -- Yes --> StartServices[Start WebServer & WebSockets]
    Success -- No --> Next[Try Next Available Match]
    Next --> Match
    
    StartServices --> Monitor[Monitor Connection]
    Monitor --> Dropped{Connection<br/>Lost?}
    Dropped -- Yes --> Scan
    Dropped -- No --> Monitor
```

## Key Features
- **Priority Based**: Connects to the primary network if available, otherwise falls back.
- **Dynamic Config**: New networks can be added by updating `wifi_networks.json` via LittleFS.
- **Fail-safe**: Retries indefinitely if no known network is found.
