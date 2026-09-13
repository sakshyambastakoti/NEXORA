#pragma once

#include <Arduino.h>
#include <WiFi.h>

class WiFiManager {
public:
    static WiFiManager& instance();

    void begin();
    void update();

    bool isConnected() const;
    int8_t getRSSI() const;
    String getIP() const;
    String getSSID() const;

    // Fallback SoftAP methods
    bool isAPActive() const { return _apActive; }
    String getAPIP() const;
    String getAPSSID() const { return String(_apSSID); }
    void startFallbackAP();
    void stopFallbackAP();

    void reconnect();

private:
    WiFiManager() = default;
    ~WiFiManager() = default;
    WiFiManager(const WiFiManager&) = delete;
    WiFiManager& operator=(const WiFiManager&) = delete;

    static constexpr uint32_t CONNECT_TIMEOUT_MS = 15000; // 15 seconds before fallback AP
    static constexpr uint32_t RETRY_INTERVAL_MS  = 12000; // 12 seconds retry interval

    uint32_t _connectionStartedMs = 0;
    uint32_t _lastAttemptMs = 0;
    bool _wasConnected = false;
    bool _apActive = false;
    char _apSSID[36] = {0};
};
