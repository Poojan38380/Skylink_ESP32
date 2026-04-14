#include "config_manager.h"
#include "logger.h"

ConfigManager::ConfigManager() {
}

bool ConfigManager::begin() {
    preferences.begin(prefNamespace, false);
    logger.info("ConfigManager initialized");
    return true;
}

bool ConfigManager::saveWiFiCredentials(const String& ssid, const String& password) {
    preferences.putString("wifi_ssid", ssid.c_str());
    preferences.putString("wifi_pass", password.c_str());
    logger.info("WiFi credentials saved");
    return true;
}

String ConfigManager::getWiFiSSID() {
    char buffer[64];
    preferences.getString("wifi_ssid", buffer, sizeof(buffer));
    return String(buffer);
}

String ConfigManager::getWiFiPassword() {
    char buffer[64];
    preferences.getString("wifi_pass", buffer, sizeof(buffer));
    return String(buffer);
}

bool ConfigManager::hasWiFiCredentials() {
    String ssid = getWiFiSSID();
    return ssid.length() > 0 && ssid != "YOUR_WIFI_SSID";
}

bool ConfigManager::saveLEDState(bool state) {
    preferences.putBool("led_state", state);
    return true;
}

bool ConfigManager::getLEDState() {
    return preferences.getBool("led_state", false);
}

bool ConfigManager::clearAll() {
    preferences.clear();
    logger.warning("All configurations cleared");
    return true;
}

bool ConfigManager::saveString(const String& key, const String& value) {
    preferences.putString(key.c_str(), value.c_str());
    return true;
}

String ConfigManager::getString(const String& key, const String& defaultValue) {
    char buffer[256];
    size_t len = preferences.getString(key.c_str(), buffer, sizeof(buffer));
    return len > 0 ? String(buffer) : defaultValue;
}

ConfigManager configManager;
