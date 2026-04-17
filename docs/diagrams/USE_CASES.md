# Use Cases

This diagram outlines the primary ways a user interacts with the Skylink system.

```mermaid
graph TD
    subgraph "Actors"
        User((Student/Developer))
        Reviewer((Professor/Reviewer))
    end

    subgraph "Skylink GCS"
        UC1([Monitor Telemetry])
        UC2([Control LED])
        UC3([View Connection Logs])
        UC4([Update WiFi Config])
        UC5([Update Firmware OTA])
    end

    User --- UC1
    User --- UC2
    User --- UC3
    User --- UC4
    User --- UC5

    Reviewer --- UC1
    Reviewer --- UC2
    Reviewer --- UC3
```

## Interactions
1. **Monitor Telemetry**: Viewing live altitude, battery, and GPS data streams.
2. **Control LED**: Sending real-time commands to toggle physical hardware.
3. **View Connection Logs**: Auditing the JSON message exchange for debugging.
4. **Update WiFi Config**: Managing network credentials via the dashboard.
5. **OTA Update**: Deploying new firmware code over the network.
