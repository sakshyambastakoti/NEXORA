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
    void drawClockPage(bool fullRedraw);
    void drawWeatherPage(bool fullRedraw);
    void drawForecastPage(bool fullRedraw);
    void drawTasksPage(bool fullRedraw);
    void drawTimerPage(bool fullRedraw);
    void drawAlarmPage(bool fullRedraw);
    void drawDeviceStatusPage(bool fullRedraw);

    UIPage _currentPage = UIPage::PAGE_CLOCK;
    uint32_t _lastPageSwitchMs = 0;
    uint32_t _lastSecondUpdateMs = 0;
    bool _needsFullRedraw = true;
};
