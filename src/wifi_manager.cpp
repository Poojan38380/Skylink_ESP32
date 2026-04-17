#include "wifi_manager.h"
#include "config.h"
#include "logger.h"
#include <Arduino.h>
#include <algorithm>

WiFiManager::WiFiManager(unsigned long reconnectInterval)
    : lastReconnectAttempt(0), reconnectInterval(reconnectInterval), currentSSID("Not Connected") {
}

bool WiFiManager::loadNetworksFromJson() {
    if (!LittleFS.begin()) {
        logger.error("Failed to mount LittleFS");
        return false;
    }

    if (!LittleFS.exists("/wifi_networks.json")) {
        logger.error("wifi_networks.json not found!");
        return false;
    }

    File file = LittleFS.open("/wifi_networks.json", "r");
    if (!file) {
        logger.error("Failed to open wifi_networks.json");
        return false;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        logger.error("Failed to parse JSON: " + String(error.c_str()));
        return false;
    }

    JsonArray networksArray = doc["networks"].as<JsonArray>();
    savedNetworks.clear();

    for (JsonObject netObj : networksArray) {
        WiFiNetwork net;
        net.ssid = netObj["ssid"].as<String>();
        net.password = netObj["password"].as<String>();
        net.priority = netObj["priority"].as<int>();
        savedNetworks.push_back(net);
    }

    // Sort by priority (ascending: 1 is highest)
    std::sort(savedNetworks.begin(), savedNetworks.end(), [](const WiFiNetwork& a, const WiFiNetwork& b) {
        return a.priority < b.priority;
    });

    logger.info("Loaded " + String(savedNetworks.size()) + " saved networks from JSON");
    return true;
}

bool WiFiManager::connectToWiFi() {
    logger.info("Scanning for available networks...");
    int numFound = WiFi.scanNetworks();
    
    if (numFound == 0) {
        logger.warning("No networks found during scan.");
        return false;
    }

    logger.info("Found " + String(numFound) + " networks. Matching against saved list...");

    for (const auto& savedNet : savedNetworks) {
        bool foundInScan = false;
        for (int i = 0; i < numFound; ++i) {
            if (WiFi.SSID(i) == savedNet.ssid) {
                foundInScan = true;
                break;
            }
        }

        if (foundInScan) {
            logger.info("Attempting connection to matching network: " + savedNet.ssid);
            WiFi.begin(savedNet.ssid.c_str(), savedNet.password.c_str());

            int attempts = 0;
            const int maxAttempts = 30; // 15 seconds
            while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
                delay(500);
                attempts++;
            }

            if (WiFi.status() == WL_CONNECTED) {
                currentSSID = savedNet.ssid;
                logger.info("WiFi connected! IP: " + WiFi.localIP().toString());
                WiFi.scanDelete();
                return true;
            } else {
                logger.warning("Failed to connect to " + savedNet.ssid);
            }
        }
    }

    logger.error("No matching saved networks were connectable.");
    WiFi.scanDelete();
    return false;
}

// Reconnect without scanning — tries each saved network directly.
// Used by handle() to avoid the 4-6 second blocking WiFi.scanNetworks() call
// that would stall the async web server and delay WebSocket handshakes.
bool WiFiManager::reconnectDirect() {
    logger.warning("WiFi disconnected. Trying saved networks directly (no scan)...");
    for (const auto& savedNet : savedNetworks) {
        logger.info("Trying: " + savedNet.ssid);
        WiFi.disconnect(true);
        delay(100);
        WiFi.begin(savedNet.ssid.c_str(), savedNet.password.c_str());

        int attempts = 0;
        const int maxAttempts = 20; // 10 seconds per network
        while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
            delay(500);
            attempts++;
        }

        if (WiFi.status() == WL_CONNECTED) {
            currentSSID = savedNet.ssid;
            logger.info("Reconnected to: " + currentSSID + " | IP: " + WiFi.localIP().toString());
            return true;
        } else {
            logger.warning("Failed to reconnect to " + savedNet.ssid);
        }
    }
    logger.error("All reconnect attempts failed.");
    return false;
}

bool WiFiManager::isWiFiConnected() {
    return WiFi.status() == WL_CONNECTED;
}

void WiFiManager::begin() {
    WiFi.mode(WIFI_STA);
    if (loadNetworksFromJson()) {
        connectToWiFi();
    } else {
        logger.error("Could not load networks from JSON. WiFi will not connect.");
    }
}

void WiFiManager::handle() {
    if (!isWiFiConnected()) {
        unsigned long now = millis();
        if (now - lastReconnectAttempt >= reconnectInterval || lastReconnectAttempt == 0) {
            lastReconnectAttempt = now;
            reconnectDirect(); // No scan — won't block the async server
        }
    }
}

String WiFiManager::getIPAddress() {
    return isWiFiConnected() ? WiFi.localIP().toString() : "Not Connected";
}

String WiFiManager::getSSID() {
    return isWiFiConnected() ? currentSSID : "Not Connected";
}

int WiFiManager::getSignalStrength() {
    return isWiFiConnected() ? WiFi.RSSI() : 0;
}

bool WiFiManager::isConnected() {
    return isWiFiConnected();
}

WiFiManager wifiManager(WIFI_RECONNECT_INTERVAL);
