#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>

class WiFiManager {
private:
    const char* ssid;
    const char* password;
    unsigned long lastReconnectAttempt;
    unsigned long reconnectInterval;
    
    bool connectToWiFi();
    bool isWiFiConnected();

public:
    WiFiManager(const char* ssid, const char* password, unsigned long reconnectInterval = 10000);
    
    void begin();
    void handle();
    
    String getIPAddress();
    String getSSID();
    int getSignalStrength();
    bool isConnected();
};

extern WiFiManager wifiManager;

#endif // WIFI_MANAGER_H
