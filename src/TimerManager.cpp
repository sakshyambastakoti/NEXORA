#include "TimerManager.h"

TimerManager& TimerManager::instance() {
    static TimerManager inst;
    return inst;
}

void TimerManager::begin() {
    Serial.println(F("[TIMER] Initializing TimerManager..."));
    _state = TimerState::STOPPED;
    _remainingSec = _totalDurationSec;
    _pendingCompleteEvent = false;
}

void TimerManager::update() {
    if (_state != TimerState::RUNNING) return;

    if (millis() - _lastTickMs >= 1000) {
        _lastTickMs += 1000;
        if (_remainingSec > 0) {
            _remainingSec--;
        }
        if (_remainingSec == 0) {
            _state = TimerState::COMPLETED;
            _pendingCompleteEvent = true;
            Serial.println(F("[TIMER] Timer reached 0! Event triggered."));
        }
    }
}

void TimerManager::setDuration(uint32_t seconds) {
    _totalDurationSec = seconds;
    _remainingSec = seconds;
    _state = TimerState::STOPPED;
    _pendingCompleteEvent = false;
    Serial.printf("[TIMER] Duration set to %u seconds.\n", seconds);
}

void TimerManager::start() {
    if (_remainingSec == 0) _remainingSec = _totalDurationSec;
    _state = TimerState::RUNNING;
    _lastTickMs = millis();
    _pendingCompleteEvent = false;
    Serial.println(F("[TIMER] Timer started."));
}

void TimerManager::pause() {
    if (_state == TimerState::RUNNING) {
        _state = TimerState::PAUSED;
        Serial.println(F("[TIMER] Timer paused."));
    }
}

void TimerManager::resume() {
    if (_state == TimerState::PAUSED) {
        _state = TimerState::RUNNING;
        _lastTickMs = millis();
        Serial.println(F("[TIMER] Timer resumed."));
    }
}

void TimerManager::stop() {
    _state = TimerState::STOPPED;
    _remainingSec = _totalDurationSec;
    Serial.println(F("[TIMER] Timer stopped and reset."));
}

void TimerManager::reset() {
    stop();
}

uint32_t TimerManager::getRemainingSeconds() const {
    return _remainingSec;
}

const char* TimerManager::getStateString() const {
    switch (_state) {
        case TimerState::STOPPED:   return "STOPPED";
        case TimerState::RUNNING:   return "RUNNING";
        case TimerState::PAUSED:    return "PAUSED";
        case TimerState::COMPLETED: return "COMPLETE";
        default:                    return "UNKNOWN";
    }
}

String TimerManager::getFormattedRemaining() const {
    uint32_t m = _remainingSec / 60;
    uint32_t s = _remainingSec % 60;
    char buf[16];
    snprintf(buf, sizeof(buf), "%02u:%02u", m, s);
    return String(buf);
}

bool TimerManager::hasPendingCompleteEvent() {
    if (_pendingCompleteEvent) {
        _pendingCompleteEvent = false;
        return true;
    }
    return false;
}
