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
    Serial.println(F("[WIFI] Initializing Wi-Fi subsystem..."));

    // Prepare default Fallback AP SSID based on unique device ID
    snprintf(_apSSID, sizeof(_apSSID), "%s-AP", g_config.deviceId);

    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);

    _connectionStartedMs = millis();
    _lastAttemptMs = millis();
    _wasConnected = false;
    _apActive = false;

    if (strlen(g_config.wifiSsid) > 0) {
        Serial.printf("[WIFI] Connecting to Station SSID: %s\n", g_config.wifiSsid);
        DeviceManager::instance().setState(DeviceState::WIFI_CONNECTING);
        WiFi.begin(g_config.wifiSsid, g_config.wifiPassword);
    } else {
        Serial.println(F("[WIFI] No Station SSID configured. Activating Fallback AP..."));
        startFallbackAP();
    }
}

void WiFiManager::update() {
    bool connected = (WiFi.status() == WL_CONNECTED);

    if (connected && !_wasConnected) {
        _wasConnected = true;
        Serial.printf("[WIFI] Connected to Station! IP: %s | RSSI: %d dBm\n", 
                      WiFi.localIP().toString().c_str(), WiFi.RSSI());
        DeviceManager::instance().setState(DeviceState::ONLINE);
        TimeManager::instance().syncNTP();
    } else if (!connected && _wasConnected) {
        _wasConnected = false;
        Serial.println(F("[WIFI] Station connection lost. Switching to OFFLINE."));
        DeviceManager::instance().setState(DeviceState::OFFLINE);
        _connectionStartedMs = millis();
        if (!_apActive) {
            startFallbackAP();
        }
    }

    // If Station is not connected, handle timeout and background retry
    if (!connected) {
        // Activate Fallback SoftAP if connection takes too long
        if (!_apActive && (millis() - _connectionStartedMs >= CONNECT_TIMEOUT_MS)) {
            Serial.println(F("[WIFI] Station connection timed out. Starting Fallback SoftAP..."));
            startFallbackAP();
        }

        // Periodic background reconnect attempt
        if (strlen(g_config.wifiSsid) > 0 && (millis() - _lastAttemptMs >= RETRY_INTERVAL_MS)) {
            _lastAttemptMs = millis();
            Serial.printf("[WIFI] Retrying Station connection to '%s'...\n", g_config.wifiSsid);
            if (!_apActive) {
                WiFi.disconnect();
            }
            WiFi.begin(g_config.wifiSsid, g_config.wifiPassword);
        }
    }
}

void WiFiManager::startFallbackAP() {
    if (_apActive) return;

    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(_apSSID, DEFAULT_AP_PASSWORD);
    _apActive = true;

    Serial.println(F("=================================================="));
    Serial.printf("[WIFI] Fallback SoftAP ACTIVE!\n");
    Serial.printf("[WIFI] SSID     : %s\n", _apSSID);
    Serial.printf("[WIFI] Password : %s\n", DEFAULT_AP_PASSWORD);
    Serial.printf("[WIFI] IP Addr  : %s\n", WiFi.softAPIP().toString().c_str());
    Serial.println(F("=================================================="));
}

void WiFiManager::stopFallbackAP() {
    if (!_apActive) return;

    WiFi.softAPdisconnect(true);
    _apActive = false;
    WiFi.mode(WIFI_STA);
    Serial.println(F("[WIFI] Fallback SoftAP stopped. Returned to pure Station mode."));
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

String WiFiManager::getAPIP() const {
    return _apActive ? WiFi.softAPIP().toString() : "";
}

String WiFiManager::getSSID() const {
    return isConnected() ? WiFi.SSID() : "";
}

void WiFiManager::reconnect() {
    Serial.println(F("[WIFI] Manual reconnect requested."));
    _wasConnected = false;
    _connectionStartedMs = millis();
    _lastAttemptMs = millis();
    WiFi.disconnect();
    if (strlen(g_config.wifiSsid) > 0) {
        WiFi.begin(g_config.wifiSsid, g_config.wifiPassword);
    }
}

