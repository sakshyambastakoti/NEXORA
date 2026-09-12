#pragma once

#include <Arduino.h>
#include "Config.h"
#include "DisplayManager.h"

class UIManager {
public:
    static UIManager& instance();

    void begin();
    void update();

    void setPage(UIPage page);
    void nextPage();
    void previousPage();
    UIPage getCurrentPage() const { return _currentPage; }

    void forceRedraw();

private:
    UIManager() = default;
    ~UIManager() = default;
    UIManager(const UIManager&) = delete;
    UIManager& operator=(const UIManager&) = delete;

    void drawCurrentPage();
    void drawClockPage();
    void drawWeatherPage();
    void drawForecastPage();
    void drawTasksPage();
    void drawTimerPage();
    void drawAlarmPage();
    void drawDeviceStatusPage();

    UIPage _currentPage = UIPage::PAGE_CLOCK;
    uint32_t _lastPageSwitchMs = 0;
    uint32_t _lastSecondUpdateMs = 0;
    bool _needsFullRedraw = true;
};
