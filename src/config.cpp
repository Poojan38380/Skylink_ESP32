#include "config.h"

// WiFi Configuration
// Loaded from data/wifi_networks.json in WiFiManager

// Pin Definitions
const int LED_BUILTIN_PIN = 2;

// Timing
const unsigned long WIFI_RECONNECT_INTERVAL = 10000; // 10 seconds
const unsigned long HEARTBEAT_INTERVAL = 5000;       // 5 seconds
