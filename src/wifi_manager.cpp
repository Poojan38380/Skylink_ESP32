#include "wifi_manager.h"
#include "config.h"
#include "logger.h"
#include <Arduino.h>
#include <algorithm>

namespace {

constexpr unsigned long WIFI_CONNECT_TIMEOUT_MS = 10000;

}  // namespace

WiFiManager::WiFiManager(unsigned long reconnectInterval)
    : lastReconnectAttempt(0),
      reconnectInterval(reconnectInterval),
      connectStartedAt(0),
      currentNetworkIndex(0),
      currentSSID("Not Connected"),
      connectState(WiFiConnectState::Idle) {
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

void WiFiManager::startConnectionAttempt() {
    if (savedNetworks.empty()) {
        logger.warning("No saved WiFi networks configured.");
        return;
    }

    if (currentNetworkIndex >= savedNetworks.size()) {
        currentNetworkIndex = 0;
    }

    const WiFiNetwork& savedNet = savedNetworks[currentNetworkIndex];
    logger.info("WiFi trying: " + savedNet.ssid);
    currentSSID = "Connecting";
    connectStartedAt = millis();
    connectState = WiFiConnectState::Connecting;
    WiFi.disconnect(false);
    WiFi.begin(savedNet.ssid.c_str(), savedNet.password.c_str());
}

void WiFiManager::advanceConnectionAttempt() {
    if (connectState != WiFiConnectState::Connecting) return;

    if (WiFi.status() == WL_CONNECTED) {
        currentSSID = savedNetworks[currentNetworkIndex].ssid;
        connectState = WiFiConnectState::Idle;
        lastReconnectAttempt = millis();
        logger.info("WiFi connected! SSID: " + currentSSID + " | IP: " + WiFi.localIP().toString());
        return;
    }

    if (millis() - connectStartedAt < WIFI_CONNECT_TIMEOUT_MS) return;

    logger.warning("WiFi connect timeout: " + savedNetworks[currentNetworkIndex].ssid);
    currentNetworkIndex++;
    if (currentNetworkIndex >= savedNetworks.size()) {
        currentNetworkIndex = 0;
        lastReconnectAttempt = millis();
        connectState = WiFiConnectState::Idle;
        currentSSID = "Not Connected";
        logger.error("All saved WiFi connection attempts failed.");
        return;
    }

    startConnectionAttempt();
}

bool WiFiManager::isWiFiConnected() {
    return WiFi.status() == WL_CONNECTED;
}

void WiFiManager::begin() {
    WiFi.mode(WIFI_STA);
    if (loadNetworksFromJson()) {
        startConnectionAttempt();
    } else {
        logger.error("Could not load networks from JSON. WiFi will not connect.");
    }
}

void WiFiManager::handle() {
    if (isWiFiConnected()) {
        connectState = WiFiConnectState::Idle;
        return;
    }

    advanceConnectionAttempt();
    if (connectState == WiFiConnectState::Connecting) return;

    unsigned long now = millis();
    if (now - lastReconnectAttempt >= reconnectInterval || lastReconnectAttempt == 0) {
        lastReconnectAttempt = now;
        logger.warning("WiFi disconnected. Starting non-blocking reconnect.");
        startConnectionAttempt();
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
