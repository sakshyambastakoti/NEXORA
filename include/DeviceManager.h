#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include "Config.h"

class DeviceManager {
public:
    static DeviceManager& instance();

    void begin();
    void update();

    DeviceState getState() const { return _state; }
    void setState(DeviceState newState);
    const char* getStateString() const;

    uint32_t getUptimeSeconds() const { return millis() / 1000; }
    uint32_t getFreeHeap() const { return ESP.getFreeHeap(); }
    uint32_t getMinFreeHeap() const { return ESP.getMinFreeHeap(); }

    // Telemetry generation
    String getStatusJson(int8_t rssi, bool mqttConnected, bool timeSynced, uint32_t weatherAgeSec);

    void restartDevice(const char* reason = "Manual restart");
    void triggerFactoryReset();

private:
    DeviceManager() = default;
    ~DeviceManager() = default;
    DeviceManager(const DeviceManager&) = delete;
    DeviceManager& operator=(const DeviceManager&) = delete;

    DeviceState _state = DeviceState::BOOTING;
    uint32_t _lastHeartbeat = 0;
};
