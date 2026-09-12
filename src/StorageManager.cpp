#include "StorageManager.h"

StorageManager& StorageManager::instance() {
    static StorageManager inst;
    return inst;
}

bool StorageManager::begin() {
    Serial.println(F("[STORAGE] Initializing NVS storage..."));
    _initialized = true;
    return true;
}

bool StorageManager::loadConfig(NEXORAConfig& config) {
    if (!_initialized) begin();

    if (!_prefs.begin(StorageKeys::NVS_CONFIG_NS, true)) { // Read-only mode
        Serial.println(F("[STORAGE] No saved configuration found. Applying defaults."));
        config.setDefaults();
        return saveConfig(config);
    }

    // Check if configuration has been initialized before
    if (!_prefs.isKey("initialized")) {
        _prefs.end();
        Serial.println(F("[STORAGE] First boot detected. Initializing default config..."));
        config.setDefaults();
        return saveConfig(config);
    }

    _prefs.getString("deviceId", config.deviceId, sizeof(config.deviceId));
    _prefs.getString("deviceName", config.deviceName, sizeof(config.deviceName));
    _prefs.getString("timezone", config.timezone, sizeof(config.timezone));
    _prefs.getString("posixTz", config.posixTz, sizeof(config.posixTz));
    _prefs.getString("locationName", config.locationName, sizeof(config.locationName));
    config.latitude = _prefs.getFloat("latitude", 27.7172f);
    config.longitude = _prefs.getFloat("longitude", 85.3240f);
    config.use24Hour = _prefs.getBool("use24Hour", true);
    config.useCelsius = _prefs.getBool("useCelsius", true);
    config.autoRotate = _prefs.getBool("autoRotate", true);
    config.pageIntervalMs = _prefs.getUInt("pageInterval", 8000);
    config.weatherIntervalMs = _prefs.getUInt("wtrInterval", 600000);
    config.statusIntervalMs = _prefs.getUInt("statInterval", 30000);

    _prefs.getString("wifiSsid", config.wifiSsid, sizeof(config.wifiSsid));
    _prefs.getString("wifiPass", config.wifiPassword, sizeof(config.wifiPassword));
    _prefs.getString("mqttBroker", config.mqttBroker, sizeof(config.mqttBroker));
    config.mqttPort = _prefs.getUShort("mqttPort", DEFAULT_MQTT_PORT);
    _prefs.getString("mqttUser", config.mqttUser, sizeof(config.mqttUser));
    _prefs.getString("mqttPass", config.mqttPassword, sizeof(config.mqttPassword));

    _prefs.end();
    Serial.printf("[STORAGE] Config loaded successfully for device: %s\n", config.deviceId);
    return true;
}

bool StorageManager::saveConfig(const NEXORAConfig& config) {
    if (!_initialized) begin();

    if (!_prefs.begin(StorageKeys::NVS_CONFIG_NS, false)) { // Read-write
        Serial.println(F("[STORAGE] ERROR: Failed to open NVS config namespace for writing."));
        return false;
    }

    _prefs.putBool("initialized", true);
    _prefs.putString("deviceId", config.deviceId);
    _prefs.putString("deviceName", config.deviceName);
    _prefs.putString("timezone", config.timezone);
    _prefs.putString("posixTz", config.posixTz);
    _prefs.putString("locationName", config.locationName);
    _prefs.putFloat("latitude", config.latitude);
    _prefs.putFloat("longitude", config.longitude);
    _prefs.putBool("use24Hour", config.use24Hour);
    _prefs.putBool("useCelsius", config.useCelsius);
    _prefs.putBool("autoRotate", config.autoRotate);
    _prefs.putUInt("pageInterval", config.pageIntervalMs);
    _prefs.putUInt("wtrInterval", config.weatherIntervalMs);
    _prefs.putUInt("statInterval", config.statusIntervalMs);

    _prefs.putString("wifiSsid", config.wifiSsid);
    _prefs.putString("wifiPass", config.wifiPassword);
    _prefs.putString("mqttBroker", config.mqttBroker);
    _prefs.putUShort("mqttPort", config.mqttPort);
    _prefs.putString("mqttUser", config.mqttUser);
    _prefs.putString("mqttPass", config.mqttPassword);

    _prefs.end();
    Serial.println(F("[STORAGE] Configuration saved to NVS."));
    return true;
}

bool StorageManager::saveWeatherCache(const String& weatherJson) {
    if (!_prefs.begin(StorageKeys::NVS_WEATHER_NS, false)) return false;
    _prefs.putString("cached_json", weatherJson);
    _prefs.putULong("cached_time", millis());
    _prefs.end();
    return true;
}

String StorageManager::loadWeatherCache() {
    if (!_prefs.begin(StorageKeys::NVS_WEATHER_NS, true)) return "";
    String result = _prefs.getString("cached_json", "");
    _prefs.end();
    return result;
}

bool StorageManager::saveTasks(const String& tasksJson) {
    if (!_prefs.begin(StorageKeys::NVS_TASKS_NS, false)) return false;
    _prefs.putString("tasks_json", tasksJson);
    _prefs.end();
    return true;
}

String StorageManager::loadTasks() {
    if (!_prefs.begin(StorageKeys::NVS_TASKS_NS, true)) return "";
    String result = _prefs.getString("tasks_json", "[]");
    _prefs.end();
    return result;
}

bool StorageManager::saveAlarms(const String& alarmsJson) {
    if (!_prefs.begin(StorageKeys::NVS_ALARMS_NS, false)) return false;
    _prefs.putString("alarms_json", alarmsJson);
    _prefs.end();
    return true;
}

String StorageManager::loadAlarms() {
    if (!_prefs.begin(StorageKeys::NVS_ALARMS_NS, true)) return "";
    String result = _prefs.getString("alarms_json", "[]");
    _prefs.end();
    return result;
}

bool StorageManager::factoryReset(NEXORAConfig& config) {
    Serial.println(F("[STORAGE] WARNING: Executing Factory Reset..."));
    
    // Clear all namespaces
    if (_prefs.begin(StorageKeys::NVS_CONFIG_NS, false)) { _prefs.clear(); _prefs.end(); }
    if (_prefs.begin(StorageKeys::NVS_TASKS_NS, false))  { _prefs.clear(); _prefs.end(); }
    if (_prefs.begin(StorageKeys::NVS_ALARMS_NS, false)) { _prefs.clear(); _prefs.end(); }
    if (_prefs.begin(StorageKeys::NVS_WEATHER_NS, false)){ _prefs.clear(); _prefs.end(); }

    config.setDefaults();
    return saveConfig(config);
}
