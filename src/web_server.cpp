#include "web_server.h"
#include "logger.h"
#include "wifi_manager.h"
#include "config.h"
#include "config_manager.h"
#include "time_sync.h"

WebServerModule::WebServerModule(int port) : server(port), port(port) {
}

void WebServerModule::updateLED(bool state) {
    digitalWrite(LED_BUILTIN_PIN, state ? HIGH : LOW);
    configManager.saveLEDState(state);
    logger.info("LED state changed: " + String(state ? "ON" : "OFF"));
}

String WebServerModule::generateDashboardHTML() {
    String html = "<!DOCTYPE html>\n";
    html += "<html>\n<head>\n";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>\n";
    html += "<title>Skylink ESP32 Dashboard</title>\n";
    html += "<style>\n";
    html += "body { font-family: Arial, sans-serif; margin: 20px; background: #f5f5f5; }\n";
    html += ".container { max-width: 800px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }\n";
    html += "h1 { color: #333; border-bottom: 2px solid #4CAF50; padding-bottom: 10px; }\n";
    html += "h2 { color: #555; margin-top: 30px; }\n";
    html += ".info-box { background: #f9f9f9; padding: 15px; margin: 10px 0; border-radius: 5px; border-left: 4px solid #4CAF50; }\n";
    html += ".label { font-weight: bold; color: #666; }\n";
    html += ".value { color: #333; margin-left: 10px; }\n";
    html += ".status-online { color: #4CAF50; }\n";
    html += ".status-offline { color: #f44336; }\n";
    html += ".btn { background: #4CAF50; color: white; padding: 10px 20px; border: none; border-radius: 4px; cursor: pointer; margin: 5px; }\n";
    html += ".btn:hover { background: #45a049; }\n";
    html += ".btn-danger { background: #f44336; }\n";
    html += ".btn-danger:hover { background: #da190b; }\n";
    html += ".nav { margin: 20px 0; }\n";
    html += ".nav a { text-decoration: none; color: #4CAF50; margin-right: 20px; font-weight: bold; }\n";
    html += "footer { margin-top: 30px; text-align: center; color: #999; font-size: 0.9em; }\n";
    html += "</style>\n";
    html += "</head>\n<body>\n";
    html += "<div class='container'>\n";
    html += "<h1>🚀 Skylink ESP32 Dashboard</h1>\n";
    html += "<div class='nav'><a href='/'>Dashboard</a><a href='/config'>Configuration</a></div>\n";
    
    // LED Control Section
    html += "<h2>💡 LED Control</h2>\n";
    html += "<div class='info-box'>\n";
    html += "<button class='btn' onclick='toggleLED(true)'>Turn ON</button>\n";
    html += "<button class='btn btn-danger' onclick='toggleLED(false)'>Turn OFF</button>\n";
    html += "<p><span class='label'>Status:</span><span class='value' id='led-status'>Loading...</span></p>\n";
    html += "</div>\n";
    
    // Device Info Section
    html += "<h2>📱 Device Information</h2>\n";
    html += "<div class='info-box'>\n";
    html += "<p><span class='label'>Chip Model:</span><span class='value'>ESP32</span></p>\n";
    html += "<p><span class='label'>CPU Frequency:</span><span class='value'>240 MHz</span></p>\n";
    html += "<p><span class='label'>Current Time:</span><span class='value'>" + timeSync.getTimestamp() + "</span></p>\n";
    html += "<p><span class='label'>Uptime:</span><span class='value' id='uptime'>" + String(millis() / 1000) + " seconds</span></p>\n";
    html += "<p><span class='label'>Free Heap:</span><span class='value' id='free-heap'>" + String(ESP.getFreeHeap()) + " bytes</span></p>\n";
    html += "</div>\n";
    
    // WiFi Status Section
    html += "<h2>📡 WiFi Status</h2>\n";
    html += "<div class='info-box'>\n";
    html += "<p><span class='label'>Status:</span><span class='value status-online' id='wifi-status'>✅ Connected</span></p>\n";
    html += "<p><span class='label'>SSID:</span><span class='value' id='wifi-ssid'>" + wifiManager.getSSID() + "</span></p>\n";
    html += "<p><span class='label'>IP Address:</span><span class='value' id='wifi-ip'>" + wifiManager.getIPAddress() + "</span></p>\n";
    html += "<p><span class='label'>Signal Strength:</span><span class='value' id='wifi-signal'>" + String(wifiManager.getSignalStrength()) + " dBm</span></p>\n";
    html += "</div>\n";
    
    html += "<footer>\n";
    html += "<p>Skylink ESP32 | Built with PlatformIO</p>\n";
    html += "</footer>\n";
    html += "</div>\n";
    
    html += "<script>\n";
    html += "function toggleLED(state) {\n";
    html += "  fetch('/api/led?state=' + state).then(r => r.json()).then(d => {\n";
    html += "    document.getElementById('led-status').textContent = state ? 'ON' : 'OFF';\n";
    html += "  });\n";
    html += "}\n";
    html += "function updateData() {\n";
    html += "  fetch('/api/wifi').then(r => r.json()).then(d => {\n";
    html += "    document.getElementById('wifi-ssid').textContent = d.ssid;\n";
    html += "    document.getElementById('wifi-ip').textContent = d.ip_address;\n";
    html += "    document.getElementById('wifi-signal').textContent = d.signal_strength_dbm + ' dBm';\n";
    html += "    document.getElementById('wifi-status').textContent = d.connected ? '✅ Connected' : '❌ Disconnected';\n";
    html += "    document.getElementById('wifi-status').className = d.connected ? 'value status-online' : 'value status-offline';\n";
    html += "  });\n";
    html += "  fetch('/api/device').then(r => r.json()).then(d => {\n";
    html += "    document.getElementById('uptime').textContent = d.uptime_seconds + ' seconds';\n";
    html += "    document.getElementById('free-heap').textContent = d.free_heap_bytes + ' bytes';\n";
    html += "  });\n";
    html += "}\n";
    html += "setInterval(updateData, 3000);\n";
    html += "updateData();\n";
    html += "</script>\n";
    
    html += "</body>\n</html>\n";
    
    return html;
}

String WebServerModule::generateConfigHTML() {
    String html = "<!DOCTYPE html>\n";
    html += "<html>\n<head>\n";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>\n";
    html += "<title>Skylink ESP32 - Configuration</title>\n";
    html += "<style>\n";
    html += "body { font-family: Arial, sans-serif; margin: 20px; background: #f5f5f5; }\n";
    html += ".container { max-width: 800px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }\n";
    html += "h1 { color: #333; border-bottom: 2px solid #4CAF50; padding-bottom: 10px; }\n";
    html += "h2 { color: #555; margin-top: 30px; }\n";
    html += ".form-group { margin: 15px 0; }\n";
    html += "label { display: block; font-weight: bold; margin-bottom: 5px; color: #666; }\n";
    html += "input[type='text'], input[type='password'] { width: 100%; padding: 8px; border: 1px solid #ddd; border-radius: 4px; box-sizing: border-box; }\n";
    html += ".btn { background: #4CAF50; color: white; padding: 10px 20px; border: none; border-radius: 4px; cursor: pointer; margin: 5px; }\n";
    html += ".btn:hover { background: #45a049; }\n";
    html += ".btn-danger { background: #f44336; }\n";
    html += ".btn-danger:hover { background: #da190b; }\n";
    html += ".nav { margin: 20px 0; }\n";
    html += ".nav a { text-decoration: none; color: #4CAF50; margin-right: 20px; font-weight: bold; }\n";
    html += ".success { color: #4CAF50; font-weight: bold; }\n";
    html += ".error { color: #f44336; font-weight: bold; }\n";
    html += "footer { margin-top: 30px; text-align: center; color: #999; font-size: 0.9em; }\n";
    html += "</style>\n";
    html += "</head>\n<body>\n";
    html += "<div class='container'>\n";
    html += "<h1>⚙️ Configuration</h1>\n";
    html += "<div class='nav'><a href='/'>Dashboard</a><a href='/config'>Configuration</a></div>\n";
    
    html += "<h2>📶 WiFi Configuration</h2>\n";
    html += "<form action='/api/wifi/config' method='get'>\n";
    html += "<div class='form-group'>\n";
    html += "<label>SSID:</label>\n";
    html += "<input type='text' name='ssid' value='" + configManager.getWiFiSSID() + "' required>\n";
    html += "</div>\n";
    html += "<div class='form-group'>\n";
    html += "<label>Password:</label>\n";
    html += "<input type='password' name='password' value='" + configManager.getWiFiPassword() + "' required>\n";
    html += "</div>\n";
    html += "<button type='submit' class='btn'>Save WiFi Settings</button>\n";
    html += "</form>\n";
    
    html += "<p>⚠️ Note: Changing WiFi settings will restart the device with new credentials.</p>\n";
    
    html += "<footer>\n";
    html += "<p>Skylink ESP32 | Built with PlatformIO</p>\n";
    html += "</footer>\n";
    html += "</div>\n";
    html += "</body>\n</html>\n";
    
    return html;
}

void WebServerModule::handleRoot() {
    logger.debug("Serving dashboard");
    server.send(200, "text/html", generateDashboardHTML());
}

void WebServerModule::handleConfigPage() {
    logger.debug("Serving config page");
    server.send(200, "text/html", generateConfigHTML());
}

void WebServerModule::handleNotFound() {
    logger.warning("404 - " + server.uri());
    server.send(404, "application/json", "{\"error\":\"Not Found\"}");
}

void WebServerModule::handleDeviceInfo() {
    logger.debug("Serving device info");
    
    String json = "{";
    json += "\"chip\":\"ESP32\",";
    json += "\"cpu_frequency_mhz\":240,";
    json += "\"uptime_seconds\":" + String(millis() / 1000) + ",";
    json += "\"free_heap_bytes\":" + String(ESP.getFreeHeap()) + ",";
    json += "\"flash_chip_size\":" + String(ESP.getFlashChipSize()) + ",";
    json += "\"sdk_version\":\"" + String(ESP.getSdkVersion()) + "\"";
    json += "}";
    
    server.send(200, "application/json", json);
}

void WebServerModule::handleWiFiStatus() {
    logger.debug("Serving WiFi status");
    
    String json = "{";
    json += "\"connected\":" + String(wifiManager.isConnected() ? "true" : "false") + ",";
    json += "\"ssid\":\"" + wifiManager.getSSID() + "\",";
    json += "\"ip_address\":\"" + wifiManager.getIPAddress() + "\",";
    json += "\"signal_strength_dbm\":" + String(wifiManager.getSignalStrength()) + ",";
    json += "\"mac_address\":\"" + WiFi.macAddress() + "\"";
    json += "}";
    
    server.send(200, "application/json", json);
}

void WebServerModule::handleSystemStatus() {
    logger.debug("Serving system status");
    
    String json = "{";
    json += "\"free_heap\":" + String(ESP.getFreeHeap()) + ",";
    json += "\"cycle_count\":" + String(ESP.getCycleCount()) + ",";
    json += "\"free_psram\":" + String(ESP.getFreePsram());
    json += "}";
    
    server.send(200, "application/json", json);
}

void WebServerModule::handleLEDControl() {
    if (server.hasArg("state")) {
        String stateStr = server.arg("state");
        bool state = (stateStr == "1" || stateStr == "true");
        
        updateLED(state);
        
        String json = "{\"success\":true,\"led_state\":" + String(state ? "true" : "false") + "}";
        server.send(200, "application/json", json);
    } else {
        server.send(400, "application/json", "{\"error\":\"Missing state parameter\"}");
    }
}

void WebServerModule::handleWiFiConfig() {
    if (server.hasArg("ssid") && server.hasArg("password")) {
        String ssid = server.arg("ssid");
        String password = server.arg("password");
        
        logger.info("Saving new WiFi credentials: " + ssid);
        configManager.saveWiFiCredentials(ssid, password);
        
        String html = "<!DOCTYPE html>\n<html><head><meta http-equiv='refresh' content='5;url=/'></head>\n";
        html += "<body style='font-family:Arial;text-align:center;padding:50px;'>\n";
        html += "<h2>✅ WiFi Settings Saved!</h2>\n";
        html += "<p>Device will restart with new WiFi credentials in 5 seconds...</p>\n";
        html += "<p><a href='/'>Return to Dashboard</a></p>\n";
        html += "</body></html>\n";
        
        server.send(200, "text/html", html);
        
        // Restart after a short delay
        delay(2000);
        ESP.restart();
    } else {
        server.send(400, "application/json", "{\"error\":\"Missing ssid or password parameter\"}");
    }
}

void WebServerModule::begin() {
    logger.info("Starting web server on port " + String(port));
    
    server.on("/", HTTP_GET, [this]() { handleRoot(); });
    server.on("/config", HTTP_GET, [this]() { handleConfigPage(); });
    server.on("/api/device", HTTP_GET, [this]() { handleDeviceInfo(); });
    server.on("/api/wifi", HTTP_GET, [this]() { handleWiFiStatus(); });
    server.on("/api/wifi/config", HTTP_GET, [this]() { handleWiFiConfig(); });
    server.on("/api/system", HTTP_GET, [this]() { handleSystemStatus(); });
    server.on("/api/led", HTTP_GET, [this]() { handleLEDControl(); });
    server.onNotFound([this]() { handleNotFound(); });
    
    server.begin();
    logger.info("Web server started!");
}

void WebServerModule::handle() {
    server.handleClient();
}

WebServerModule webServerModule(80);
