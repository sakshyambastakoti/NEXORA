#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

constexpr size_t MAX_ALARMS = 5;

struct NEXORAAlarm {
    char id[16];
    char name[32];
    uint8_t hour;
    uint8_t minute;
    bool enabled;
    uint8_t repeatMask; // Bit 0=Sun, 1=Mon, 2=Tue, 3=Wed, 4=Thu, 5=Fri, 6=Sat
};

class AlarmManager {
public:
    static AlarmManager& instance();

    void begin();
    void update();

    bool addOrUpdateAlarm(const char* id, const char* name, uint8_t hour, uint8_t minute, bool enabled, uint8_t repeatMask);
    bool deleteAlarm(const char* id);
    bool toggleAlarm(const char* id);

    size_t getAlarmCount() const { return _alarmCount; }
    const NEXORAAlarm* getAlarm(size_t idx) const {
        if (idx < _alarmCount) return &_alarms[idx];
        return nullptr;
    }

    String serializeAlarmsJson() const;
    bool deserializeAlarmsJson(const char* jsonStr);

    bool hasPendingTriggerEvent(String& outAlarmId);

private:
    AlarmManager() = default;
    ~AlarmManager() = default;
    AlarmManager(const AlarmManager&) = delete;
    AlarmManager& operator=(const AlarmManager&) = delete;

    void saveToStorage();
    void checkAlarms();

    NEXORAAlarm _alarms[MAX_ALARMS];
    size_t _alarmCount = 0;
    int _lastCheckedMinute = -1;
    String _pendingAlarmId = "";
};
