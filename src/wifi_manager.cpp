#include "wifi_manager.h"
#include "config.h"
#include "logger.h"
#include <Arduino.h>

WiFiManager::WiFiManager(const char* ssid, const char* password, unsigned long reconnectInterval)
    : ssid(ssid), password(password), lastReconnectAttempt(0), reconnectInterval(reconnectInterval) {
}

bool WiFiManager::connectToWiFi() {
    logger.info("Scanning for available networks...");
    
    int n = WiFi.scanNetworks();
    if (n == 0) {
        logger.error("No networks found! Make sure hotspot is ON and broadcasting 2.4GHz");
    } else {
        logger.info("Found " + String(n) + " networks:");
        bool found = false;
        for (int i = 0; i < n; ++i) {
            logger.info("  " + String(i+1) + ": " + WiFi.SSID(i) + " (" + WiFi.RSSI(i) + " dBm)");
            if (WiFi.SSID(i) == ssid) {
                found = true;
                logger.info("  >>> Found target: " + String(ssid));
            }
        }
        if (!found) {
            logger.error("Target network '" + String(ssid) + "' NOT FOUND in scan!");
            logger.error("Is hotspot ON? Check if it's broadcasting 2.4GHz");
        }
    }
    
    WiFi.scanDelete();
    delay(1000);
    
    logger.info("Connecting to WiFi: " + String(ssid));
    
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    const int maxAttempts = 40;  // Increased from 20 to 40 (20 seconds total)
    
    while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
        delay(500);
        logger.debug("Connecting... Attempt " + String(attempts + 1) + "/" + String(maxAttempts) + " | Status: " + String(WiFi.status()));
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        logger.info("WiFi connected! IP: " + WiFi.localIP().toString());
        logger.info("Signal strength: " + String(WiFi.RSSI()) + " dBm");
        return true;
    } else {
        logger.error("Failed to connect to WiFi. Status: " + String(WiFi.status()));
        logger.error("Check: Is hotspot on 2.4GHz? Is password correct?");
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
