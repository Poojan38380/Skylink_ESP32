#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>

class ConfigManager {
private:
    Preferences preferences;
    const char* prefNamespace = "skylink";
    
public:
    ConfigManager();
    
    bool begin();
    
    // WiFi Settings
    bool saveWiFiCredentials(const String& ssid, const String& password);
    String getWiFiSSID();
    String getWiFiPassword();
    bool hasWiFiCredentials();
    
    // LED Settings
    bool saveLEDState(bool state);
    bool getLEDState();
    
    // General
    bool clearAll();
    bool saveString(const String& key, const String& value);
    String getString(const String& key, const String& defaultValue = "");
};

extern ConfigManager configManager;

#endif // CONFIG_MANAGER_H
