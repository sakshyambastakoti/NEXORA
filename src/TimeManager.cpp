#include "TimeManager.h"
#include <WiFi.h>

extern NEXORAConfig g_config;

TimeManager& TimeManager::instance() {
    static TimeManager inst;
    return inst;
}

void TimeManager::begin() {
    Serial.println(F("[TIME] Initializing TimeManager..."));
    setTimezone(g_config.posixTz);
}

void TimeManager::setTimezone(const char* posixTz) {
    Serial.printf("[TIME] Setting timezone POSIX string: %s\n", posixTz);
    configTzTime(posixTz, "pool.ntp.org", "time.google.com", "time.cloudflare.com");
}

void TimeManager::syncNTP() {
    if (WiFi.status() != WL_CONNECTED) return;
    
    _lastSyncAttempt = millis();
    Serial.println(F("[TIME] Triggering NTP time synchronization..."));
    configTzTime(g_config.posixTz, "pool.ntp.org", "time.google.com", "time.cloudflare.com");
    
    // Check if time is valid
    time_t now = time(nullptr);
    if (now > 1700000000) { // Valid timestamp past Nov 2023
        _synced = true;
        Serial.printf("[TIME] NTP synchronized successfully! Current Epoch: %ld\n", (long)now);
    }
}

void TimeManager::update() {
    uint32_t interval = _synced ? SYNC_SUCCESS_INTERVAL : SYNC_RETRY_INTERVAL;
    if (WiFi.status() == WL_CONNECTED && (millis() - _lastSyncAttempt >= interval || _lastSyncAttempt == 0)) {
        syncNTP();
    }

    if (!_synced) {
        time_t now = time(nullptr);
        if (now > 1700000000) {
            _synced = true;
            Serial.printf("[TIME] Time sync detected! Current Epoch: %ld\n", (long)now);
        }
    }
}

time_t TimeManager::getEpoch() const {
    return time(nullptr);
}

String TimeManager::getTimeString(bool includeSeconds, bool use24H) const {
    time_t now = time(nullptr);
    struct tm timeinfo;
    if (!localtime_r(&now, &timeinfo) || now < 100000) {
        return includeSeconds ? "12:00:00" : "12:00";
    }

    char buf[16];
    if (use24H) {
        if (includeSeconds) {
            snprintf(buf, sizeof(buf), "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        } else {
            snprintf(buf, sizeof(buf), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
        }
    } else {
        int h = timeinfo.tm_hour;
        const char* ampm = (h >= 12) ? "PM" : "AM";
        h = h % 12;
        if (h == 0) h = 12;
        if (includeSeconds) {
            snprintf(buf, sizeof(buf), "%02d:%02d:%02d %s", h, timeinfo.tm_min, timeinfo.tm_sec, ampm);
        } else {
            snprintf(buf, sizeof(buf), "%02d:%02d %s", h, timeinfo.tm_min, ampm);
        }
    }
    return String(buf);
}

String TimeManager::getDateString() const {
    time_t now = time(nullptr);
    struct tm timeinfo;
    if (!localtime_r(&now, &timeinfo) || now < 100000) {
        return "Friday, 12 September 2026";
    }

    const char* months[] = {
        "January", "February", "March", "April", "May", "June",
        "July", "August", "September", "October", "November", "December"
    };
    const char* days[] = {
        "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
    };

    char buf[48];
    snprintf(buf, sizeof(buf), "%s, %d %s %d",
             days[timeinfo.tm_wday],
             timeinfo.tm_mday,
             months[timeinfo.tm_mon],
             timeinfo.tm_year + 1900);
    return String(buf);
}

String TimeManager::getDayOfWeek() const {
    time_t now = time(nullptr);
    struct tm timeinfo;
    if (!localtime_r(&now, &timeinfo) || now < 100000) {
        return "Friday";
    }
    const char* days[] = {
        "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
    };
    return String(days[timeinfo.tm_wday]);
}

int TimeManager::getHour() const {
    time_t now = time(nullptr);
    struct tm timeinfo;
    if (!localtime_r(&now, &timeinfo)) return 12;
    return timeinfo.tm_hour;
}

int TimeManager::getMinute() const {
    time_t now = time(nullptr);
    struct tm timeinfo;
    if (!localtime_r(&now, &timeinfo)) return 0;
    return timeinfo.tm_min;
}

int TimeManager::getSecond() const {
    time_t now = time(nullptr);
    struct tm timeinfo;
    if (!localtime_r(&now, &timeinfo)) return 0;
    return timeinfo.tm_sec;
}
