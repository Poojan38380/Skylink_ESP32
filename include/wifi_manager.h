#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>

#include <vector>
#include <LittleFS.h>
#include <ArduinoJson.h>

struct WiFiNetwork {
    String ssid;
    String password;
    int priority;
};

enum class WiFiConnectState : uint8_t {
    Idle = 0,
    Connecting
};

class WiFiManager {
private:
    std::vector<WiFiNetwork> savedNetworks;
    unsigned long lastReconnectAttempt;
    unsigned long reconnectInterval;
    unsigned long connectStartedAt;
    size_t currentNetworkIndex;
    String currentSSID;
    WiFiConnectState connectState;
    
    bool loadNetworksFromJson();
    void startConnectionAttempt();
    void advanceConnectionAttempt();
    bool isWiFiConnected();

public:
    WiFiManager(unsigned long reconnectInterval = 10000);
    
    void begin();
    void handle();
    
    String getIPAddress();
    String getSSID();
    int getSignalStrength();
    bool isConnected();
};

extern WiFiManager wifiManager;

#endif // WIFI_MANAGER_H
