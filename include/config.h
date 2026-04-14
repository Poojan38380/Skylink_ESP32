#ifndef CONFIG_H
#define CONFIG_H

// WiFi Configuration
extern const char *WIFI_SSID;
extern const char *WIFI_PASSWORD;

// Pin Definitions
extern const int LED_BUILTIN_PIN;

// Timing
extern const unsigned long WIFI_RECONNECT_INTERVAL;
extern const unsigned long HEARTBEAT_INTERVAL;

#endif // CONFIG_H
