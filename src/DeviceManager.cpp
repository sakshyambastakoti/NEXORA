#include "DeviceManager.h"
#include "StorageManager.h"

extern NEXORAConfig g_config;

DeviceManager& DeviceManager::instance() {
    static DeviceManager inst;
    return inst;
}

void DeviceManager::begin() {
    Serial.println(F("[DEVICE] DeviceManager initialized."));
    setState(DeviceState::INITIALIZING);
}

void DeviceManager::update() {
    // Periodic internal health check (every 10 seconds)
    if (millis() - _lastHeartbeat >= 10000) {
        _lastHeartbeat = millis();
        // Check for low memory warning
        if (getFreeHeap() < 30000) {
            Serial.printf("[DEVICE] WARNING: Low free heap: %u bytes\n", getFreeHeap());
        }
    }
}

void DeviceManager::setState(DeviceState newState) {
    if (_state == newState) return;
    DeviceState oldState = _state;
    _state = newState;
    Serial.printf("[STATE] %s -> %s\n", 
                  oldState == DeviceState::BOOTING ? "BOOTING" :
                  oldState == DeviceState::INITIALIZING ? "INITIALIZING" :
                  oldState == DeviceState::WIFI_CONNECTING ? "WIFI_CONNECTING" :
                  oldState == DeviceState::ONLINE ? "ONLINE" :
                  oldState == DeviceState::OFFLINE ? "OFFLINE" :
                  oldState == DeviceState::ERROR ? "ERROR" : "OTA_UPDATE",
                  getStateString());
}

const char* DeviceManager::getStateString() const {
    switch (_state) {
        case DeviceState::BOOTING:         return "BOOTING";
        case DeviceState::INITIALIZING:    return "INITIALIZING";
        case DeviceState::WIFI_CONNECTING: return "CONNECTING";
        case DeviceState::ONLINE:          return "ONLINE";
        case DeviceState::OFFLINE:         return "OFFLINE";
        case DeviceState::ERROR:           return "ERROR";
        case DeviceState::OTA_UPDATE:      return "OTA_UPDATE";
        default:                           return "UNKNOWN";
    }
}

String DeviceManager::getStatusJson(int8_t rssi, bool mqttConnected, bool timeSynced, uint32_t weatherAgeSec) {
    JsonDocument doc;
    doc["device_id"] = g_config.deviceId;
    doc["device_name"] = g_config.deviceName;
    doc["firmware"] = NEXORA_FIRMWARE_VERSION;
    doc["state"] = getStateString();
    doc["uptime"] = getUptimeSeconds();
    doc["free_heap"] = getFreeHeap();
    doc["min_free_heap"] = getMinFreeHeap();
    doc["wifi_rssi"] = rssi;
    doc["mqtt_connected"] = mqttConnected;
    doc["time_synced"] = timeSynced;
    doc["weather_age"] = weatherAgeSec;

    String output;
    serializeJson(doc, output);
    return output;
}

void DeviceManager::restartDevice(const char* reason) {
    Serial.printf("[DEVICE] Reboot requested: %s. Restarting in 500ms...\n", reason);
    delay(500);
    ESP.restart();
}

void DeviceManager::triggerFactoryReset() {
    Serial.println(F("[DEVICE] Executing complete factory reset..."));
    StorageManager::instance().factoryReset(g_config);
    delay(500);
    ESP.restart();
}
