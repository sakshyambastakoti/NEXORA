#pragma once

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <WebServer.h>
#include <Update.h>

class OTAManager {
public:
    static OTAManager& instance();

    void begin();
    void update();

    bool isUpdating() const { return _isUpdating; }

private:
    OTAManager();
    ~OTAManager() = default;
    OTAManager(const OTAManager&) = delete;
    OTAManager& operator=(const OTAManager&) = delete;

    void setupWebServer();
    void handleRoot();
    void handleSaveWifi();
    void handleUpdatePage();
    void handleUpdateDone();
    void handleUpdateUpload();

    WebServer _server;
    bool _initialized = false;
    bool _isUpdating = false;
    size_t _uploadTotalSize = 0;
    int _lastProgressPct = -1;
};

