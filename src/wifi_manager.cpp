#include "wifi_manager.h"
#include "config.h"
#include "logger.h"
#include <Arduino.h>

WiFiManager::WiFiManager(const char* ssid, const char* password, unsigned long reconnectInterval)
    : ssid(ssid), password(password), lastReconnectAttempt(0), reconnectInterval(reconnectInterval) {
}

bool WiFiManager::connectToWiFi() {
    logger.info("Connecting to WiFi: " + String(ssid));
    
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    const int maxAttempts = 20;
    
    while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
        delay(500);
        logger.debug("Connecting... (" + String(attempts + 1) + "/" + String(maxAttempts) + ")");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        logger.info("WiFi connected! IP: " + WiFi.localIP().toString());
        return true;
    } else {
        logger.error("Failed to connect to WiFi");
        return false;
    }
}

bool WiFiManager::isWiFiConnected() {
    return WiFi.status() == WL_CONNECTED;
}

void WiFiManager::begin() {
    WiFi.mode(WIFI_STA);
    connectToWiFi();
}

void WiFiManager::handle() {
    if (!isWiFiConnected()) {
        unsigned long now = millis();
        if (now - lastReconnectAttempt >= reconnectInterval || lastReconnectAttempt == 0) {
            lastReconnectAttempt = now;
            logger.warning("WiFi disconnected, attempting to reconnect...");
            connectToWiFi();
        }
    }
}

String WiFiManager::getIPAddress() {
    return isWiFiConnected() ? WiFi.localIP().toString() : "Not Connected";
}

String WiFiManager::getSSID() {
    return isWiFiConnected() ? WiFi.SSID() : "Not Connected";
}

int WiFiManager::getSignalStrength() {
    return isWiFiConnected() ? WiFi.RSSI() : 0;
}

bool WiFiManager::isConnected() {
    return isWiFiConnected();
}

WiFiManager wifiManager(WIFI_SSID, WIFI_PASSWORD, WIFI_RECONNECT_INTERVAL);
