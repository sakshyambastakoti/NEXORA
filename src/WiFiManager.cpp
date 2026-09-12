#include "WiFiManager.h"
#include "Config.h"
#include "DeviceManager.h"
#include "TimeManager.h"

extern NEXORAConfig g_config;

WiFiManager& WiFiManager::instance() {
    static WiFiManager inst;
    return inst;
}

void WiFiManager::begin() {
    Serial.println(F("[WIFI] Initializing Wi-Fi station..."));
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);

    if (strlen(g_config.wifiSsid) > 0) {
        Serial.printf("[WIFI] Connecting to SSID: %s\n", g_config.wifiSsid);
        DeviceManager::instance().setState(DeviceState::WIFI_CONNECTING);
        WiFi.begin(g_config.wifiSsid, g_config.wifiPassword);
    } else {
        Serial.println(F("[WIFI] No SSID configured. Running in OFFLINE mode."));
        DeviceManager::instance().setState(DeviceState::OFFLINE);
    }
    _lastAttemptMs = millis();
}

void WiFiManager::update() {
    bool connected = (WiFi.status() == WL_CONNECTED);

    if (connected && !_wasConnected) {
        _wasConnected = true;
        Serial.printf("[WIFI] Connected! IP: %s | RSSI: %d dBm\n", 
                      WiFi.localIP().toString().c_str(), WiFi.RSSI());
        TimeManager::instance().syncNTP();
    } else if (!connected && _wasConnected) {
        _wasConnected = false;
        Serial.println(F("[WIFI] Connection lost. Fallback to OFFLINE mode."));
        DeviceManager::instance().setState(DeviceState::OFFLINE);
    }

    // Periodic reconnect check if disconnected
    if (!connected && strlen(g_config.wifiSsid) > 0) {
        if (millis() - _lastAttemptMs >= _retryIntervalMs) {
            _lastAttemptMs = millis();
            Serial.println(F("[WIFI] Attempting Wi-Fi reconnect..."));
            WiFi.disconnect();
            WiFi.begin(g_config.wifiSsid, g_config.wifiPassword);
        }
    }
}

bool WiFiManager::isConnected() const {
    return (WiFi.status() == WL_CONNECTED);
}

int8_t WiFiManager::getRSSI() const {
    return isConnected() ? WiFi.RSSI() : -100;
}

String WiFiManager::getIP() const {
    return isConnected() ? WiFi.localIP().toString() : "0.0.0.0";
}

String WiFiManager::getSSID() const {
    return isConnected() ? WiFi.SSID() : "";
}

void WiFiManager::reconnect() {
    Serial.println(F("[WIFI] Manual reconnect requested."));
    WiFi.disconnect();
    _lastAttemptMs = 0; // Trigger immediately in update()
}
