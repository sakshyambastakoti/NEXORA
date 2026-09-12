#pragma once

#include <Arduino.h>

enum class TimerState : uint8_t {
    STOPPED,
    RUNNING,
    PAUSED,
    COMPLETED
};

class TimerManager {
public:
    static TimerManager& instance();

    void begin();
    void update();

    void setDuration(uint32_t seconds);
    void start();
    void pause();
    void resume();
    void stop();
    void reset();

    TimerState getState() const { return _state; }
    const char* getStateString() const;
    uint32_t getRemainingSeconds() const;
    uint32_t getTotalDuration() const { return _totalDurationSec; }
    String getFormattedRemaining() const;

    bool hasPendingCompleteEvent();

private:
    TimerManager() = default;
    ~TimerManager() = default;
    TimerManager(const TimerManager&) = delete;
    TimerManager& operator=(const TimerManager&) = delete;

    TimerState _state = TimerState::STOPPED;
    uint32_t _totalDurationSec = 1500; // 25 min default (Pomodoro style)
    uint32_t _remainingSec = 1500;
    uint32_t _lastTickMs = 0;
    bool _pendingCompleteEvent = false;
};
