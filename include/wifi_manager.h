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

class WiFiManager {
private:
    std::vector<WiFiNetwork> savedNetworks;
    unsigned long lastReconnectAttempt;
    unsigned long reconnectInterval;
    String currentSSID;
    
    bool loadNetworksFromJson();
    bool connectToWiFi();      // Full scan + match (used on first boot)
    bool reconnectDirect();    // No scan, try saved list directly (used on reconnects)
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
