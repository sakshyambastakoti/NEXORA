#pragma once

#include <Arduino.h>
#include <ArduinoOTA.h>

class OTAManager {
public:
    static OTAManager& instance();

    void begin();
    void update();

    bool isUpdating() const { return _isUpdating; }

private:
    OTAManager() = default;
    ~OTAManager() = default;
    OTAManager(const OTAManager&) = delete;
    OTAManager& operator=(const OTAManager&) = delete;

    bool _initialized = false;
    bool _isUpdating = false;
};
