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

    void reconnect();

private:
    WiFiManager() = default;
    ~WiFiManager() = default;
    WiFiManager(const WiFiManager&) = delete;
    WiFiManager& operator=(const WiFiManager&) = delete;

    uint32_t _lastAttemptMs = 0;
    uint32_t _retryIntervalMs = 10000;
    bool _wasConnected = false;
};
