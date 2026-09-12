#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include "Config.h"

class StorageManager {
public:
    static StorageManager& instance();

    bool begin();
    
    // System configuration
    bool loadConfig(NEXORAConfig& config);
    bool saveConfig(const NEXORAConfig& config);
    
    // Cached weather persistence
    bool saveWeatherCache(const String& weatherJson);
    String loadWeatherCache();

    // Tasks persistence (JSON serialized)
    bool saveTasks(const String& tasksJson);
    String loadTasks();

    // Alarms persistence (JSON serialized)
    bool saveAlarms(const String& alarmsJson);
    String loadAlarms();

    // Factory reset
    bool factoryReset(NEXORAConfig& config);

private:
    StorageManager() = default;
    ~StorageManager() = default;
    StorageManager(const StorageManager&) = delete;
    StorageManager& operator=(const StorageManager&) = delete;

    Preferences _prefs;
    bool _initialized = false;
};
