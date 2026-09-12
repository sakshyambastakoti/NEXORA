#include "OTAManager.h"
#include "Config.h"
#include "DeviceManager.h"
#include "DisplayManager.h"
#include "WiFiManager.h"

extern NEXORAConfig g_config;

OTAManager& OTAManager::instance() {
    static OTAManager inst;
    return inst;
}

void OTAManager::begin() {
    if (_initialized) return;

    ArduinoOTA.setHostname(g_config.deviceId);
    ArduinoOTA.setPassword(DEFAULT_OTA_PASSWORD);

    ArduinoOTA.onStart([this]() {
        _isUpdating = true;
        DeviceManager::instance().setState(DeviceState::OTA_UPDATE);
        Serial.println(F("[OTA] Firmware update process started."));
        DisplayManager::instance().showBootScreen("FIRMWARE UPDATE IN PROGRESS", 0);
    });

    ArduinoOTA.onEnd([]() {
        Serial.println(F("\n[OTA] Firmware update completed successfully! Rebooting..."));
        DisplayManager::instance().showBootScreen("UPDATE COMPLETE! REBOOTING...", 100);
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        unsigned int pct = (progress / (total / 100));
        DisplayManager::instance().showBootScreen("RECEIVING FIRMWARE DATA...", pct);
        Serial.printf("[OTA] Progress: %u%%\r", pct);
    });

    ArduinoOTA.onError([this](ota_error_t error) {
        _isUpdating = false;
        DeviceManager::instance().setState(DeviceState::ONLINE);
        Serial.printf("\n[OTA] Error [%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println(F("Auth Failed"));
        else if (error == OTA_BEGIN_ERROR) Serial.println(F("Begin Failed"));
        else if (error == OTA_CONNECT_ERROR) Serial.println(F("Connect Failed"));
        else if (error == OTA_RECEIVE_ERROR) Serial.println(F("Receive Failed"));
        else if (error == OTA_END_ERROR) Serial.println(F("End Failed"));
        DisplayManager::instance().showBootScreen("OTA UPDATE FAILED!", 0);
    });

    ArduinoOTA.begin();
    _initialized = true;
    Serial.println(F("[OTA] OTAManager service started."));
}

void OTAManager::update() {
    if (WiFiManager::instance().isConnected()) {
        if (!_initialized) begin();
        ArduinoOTA.handle();
    }
}
