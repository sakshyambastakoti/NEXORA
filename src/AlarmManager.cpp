#include "AlarmManager.h"
#include "StorageManager.h"
#include "TimeManager.h"

AlarmManager& AlarmManager::instance() {
    static AlarmManager inst;
    return inst;
}

void AlarmManager::begin() {
    Serial.println(F("[ALARM] Initializing AlarmManager..."));
    _alarmCount = 0;
    String loaded = StorageManager::instance().loadAlarms();
    if (loaded.length() > 5) {
        deserializeAlarmsJson(loaded.c_str());
    }

    if (_alarmCount == 0) {
        // Default sample alarm (Weekdays 07:00 AM)
        addOrUpdateAlarm("alm_1", "Morning Wakeup", 7, 0, false, 0b00111110);
    }
}

void AlarmManager::update() {
    time_t now = TimeManager::instance().getEpoch();
    struct tm ti;
    if (!localtime_r(&now, &ti)) return;

    if (ti.tm_min != _lastCheckedMinute) {
        _lastCheckedMinute = ti.tm_min;
        checkAlarms();
    }
}

void AlarmManager::checkAlarms() {
    time_t now = TimeManager::instance().getEpoch();
    struct tm ti;
    if (!localtime_r(&now, &ti)) return;

    uint8_t todayBit = 1 << ti.tm_wday; // 0=Sun .. 6=Sat

    for (size_t i = 0; i < _alarmCount; i++) {
        const NEXORAAlarm& a = _alarms[i];
        if (!a.enabled) continue;

        if (a.hour == ti.tm_hour && a.minute == ti.tm_min) {
            // Check day mask (if repeatMask == 0, one-shot alarm)
            if (a.repeatMask == 0 || (a.repeatMask & todayBit)) {
                Serial.printf("[ALARM] Alarm triggered: %s (%s) at %02d:%02d\n", a.id, a.name, a.hour, a.minute);
                _pendingAlarmId = a.id;
            }
        }
    }
}

bool AlarmManager::addOrUpdateAlarm(const char* id, const char* name, uint8_t hour, uint8_t minute, bool enabled, uint8_t repeatMask) {
    for (size_t i = 0; i < _alarmCount; i++) {
        if (strcmp(_alarms[i].id, id) == 0) {
            strncpy(_alarms[i].name, name, sizeof(_alarms[i].name));
            _alarms[i].hour = hour;
            _alarms[i].minute = minute;
            _alarms[i].enabled = enabled;
            _alarms[i].repeatMask = repeatMask;
            saveToStorage();
            return true;
        }
    }

    if (_alarmCount >= MAX_ALARMS) return false;

    NEXORAAlarm& a = _alarms[_alarmCount++];
    strncpy(a.id, id, sizeof(a.id));
    strncpy(a.name, name, sizeof(a.name));
    a.hour = hour;
    a.minute = minute;
    a.enabled = enabled;
    a.repeatMask = repeatMask;

    saveToStorage();
    Serial.printf("[ALARM] Configured alarm %s: %02d:%02d\n", id, hour, minute);
    return true;
}

bool AlarmManager::deleteAlarm(const char* id) {
    for (size_t i = 0; i < _alarmCount; i++) {
        if (strcmp(_alarms[i].id, id) == 0) {
            for (size_t j = i; j < _alarmCount - 1; j++) {
                _alarms[j] = _alarms[j + 1];
            }
            _alarmCount--;
            saveToStorage();
            return true;
        }
    }
    return false;
}

bool AlarmManager::toggleAlarm(const char* id) {
    for (size_t i = 0; i < _alarmCount; i++) {
        if (strcmp(_alarms[i].id, id) == 0) {
            _alarms[i].enabled = !_alarms[i].enabled;
            saveToStorage();
            return true;
        }
    }
    return false;
}

String AlarmManager::serializeAlarmsJson() const {
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();
    for (size_t i = 0; i < _alarmCount; i++) {
        JsonObject obj = arr.add<JsonObject>();
        obj["id"] = _alarms[i].id;
        obj["name"] = _alarms[i].name;
        obj["hour"] = _alarms[i].hour;
        obj["minute"] = _alarms[i].minute;
        obj["enabled"] = _alarms[i].enabled;
        obj["repeat_mask"] = _alarms[i].repeatMask;
    }
    String out;
    serializeJson(doc, out);
    return out;
}

bool AlarmManager::deserializeAlarmsJson(const char* jsonStr) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, jsonStr);
    if (err) return false;

    JsonArray arr = doc.as<JsonArray>();
    if (arr.isNull()) return false;

    _alarmCount = 0;
    for (JsonObject obj : arr) {
        if (_alarmCount >= MAX_ALARMS) break;
        NEXORAAlarm& a = _alarms[_alarmCount++];
        strncpy(a.id, obj["id"] | "alm", sizeof(a.id));
        strncpy(a.name, obj["name"] | "Alarm", sizeof(a.name));
        a.hour = obj["hour"] | 0;
        a.minute = obj["minute"] | 0;
        a.enabled = obj["enabled"] | false;
        a.repeatMask = obj["repeat_mask"] | 0;
    }
    return true;
}

bool AlarmManager::hasPendingTriggerEvent(String& outAlarmId) {
    if (_pendingAlarmId.length() > 0) {
        outAlarmId = _pendingAlarmId;
        _pendingAlarmId = "";
        return true;
    }
    return false;
}

void AlarmManager::saveToStorage() {
    StorageManager::instance().saveAlarms(serializeAlarmsJson());
}
