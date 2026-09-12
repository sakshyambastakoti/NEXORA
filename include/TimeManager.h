#pragma once

#include <Arduino.h>
#include <time.h>
#include "Config.h"

class TimeManager {
public:
    static TimeManager& instance();

    void begin();
    void update();

    void syncNTP();
    void setTimezone(const char* posixTz);

    bool isSynced() const { return _synced; }
    time_t getEpoch() const;

    String getTimeString(bool includeSeconds = true, bool use24H = true) const;
    String getDateString() const;
    String getDayOfWeek() const;

    int getHour() const;
    int getMinute() const;
    int getSecond() const;

private:
    TimeManager() = default;
    ~TimeManager() = default;
    TimeManager(const TimeManager&) = delete;
    TimeManager& operator=(const TimeManager&) = delete;

    bool _synced = false;
    uint32_t _lastSyncAttempt = 0;
    static constexpr uint32_t SYNC_RETRY_INTERVAL = 60000;    // 1 min retry if failed
    static constexpr uint32_t SYNC_SUCCESS_INTERVAL = 3600000; // 1 hr sync once synced
};
