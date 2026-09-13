#include "UIManager.h"
#include "TimeManager.h"
#include "WeatherManager.h"
#include "TaskManager.h"
#include "TimerManager.h"
#include "AlarmManager.h"
#include "DeviceManager.h"
#include "WiFiManager.h"
#include <WiFi.h>

extern NEXORAConfig g_config;

UIManager& UIManager::instance() {
    static UIManager inst;
    return inst;
}

void UIManager::begin() {
    Serial.println(F("[UI] Initializing UIManager..."));
    _currentPage = UIPage::PAGE_CLOCK;
    _lastPageSwitchMs = millis();
    _lastSecondUpdateMs = millis();
    _needsFullRedraw = true;
}

void UIManager::setPage(UIPage page) {
    if (page >= UIPage::PAGE_COUNT) page = UIPage::PAGE_CLOCK;
    _currentPage = page;
    _lastPageSwitchMs = millis();
    _needsFullRedraw = true;
}

void UIManager::nextPage() {
    int next = static_cast<int>(_currentPage) + 1;
    if (next >= static_cast<int>(UIPage::PAGE_COUNT)) {
        next = 0;
    }
    setPage(static_cast<UIPage>(next));
}

void UIManager::previousPage() {
    int prev = static_cast<int>(_currentPage) - 1;
    if (prev < 0) {
        prev = static_cast<int>(UIPage::PAGE_COUNT) - 1;
    }
    setPage(static_cast<UIPage>(prev));
}

void UIManager::forceRedraw() {
    _needsFullRedraw = true;
}

void UIManager::update() {
    // Guard against drawing while firmware update is in progress
    if (DeviceManager::instance().getState() == DeviceState::OTA_UPDATE) {
        return;
    }

    // 1. Auto-rotation handling (only if explicitly enabled and interval >= 5s)
    if (g_config.autoRotate && g_config.pageIntervalMs >= 5000) {
        if (millis() - _lastPageSwitchMs >= g_config.pageIntervalMs) {
            nextPage();
        }
    }

    // 2. Per-second refresh for clock, timer, and system stats
    if (millis() - _lastSecondUpdateMs >= 1000 || _needsFullRedraw) {
        _lastSecondUpdateMs = millis();
        drawCurrentPage();
    }
}

void UIManager::drawCurrentPage() {
    bool fullRedraw = _needsFullRedraw;
    _needsFullRedraw = false;

    DisplayManager& dm = DisplayManager::instance();
    LGFX_NEXORA& lcd = dm.getDisplay();

    // WiFi and status info
    WiFiManager& wifi = WiFiManager::instance();
    int8_t rssi = wifi.isConnected() ? wifi.getRSSI() : (wifi.isAPActive() ? -65 : -100);
    bool mqttOk = (DeviceManager::instance().getState() == DeviceState::ONLINE);
    String timeMini = TimeManager::instance().getTimeString(false, g_config.use24Hour);

    const char* title = "NEXORA";
    switch (_currentPage) {
        case UIPage::PAGE_CLOCK:         title = "MAIN DASHBOARD"; break;
        case UIPage::PAGE_WEATHER:       title = "WEATHER"; break;
        case UIPage::PAGE_FORECAST:      title = "3-DAY FORECAST"; break;
        case UIPage::PAGE_TASKS:         title = "MY TASKS"; break;
        case UIPage::PAGE_TIMER:         title = "TIMER"; break;
        case UIPage::PAGE_ALARM:         title = "SCHEDULED ALARMS"; break;
        case UIPage::PAGE_DEVICE_STATUS: title = "SYSTEM STATUS"; break;
        default:                         title = "NEXORA"; break;
    }

    if (fullRedraw) {
        dm.renderHeader(title, _currentPage, rssi, mqttOk, timeMini.c_str());
    } else {
        // Mini clock update on header (clean overwrite)
        lcd.setTextDatum(textdatum_t::middle_right);
        lcd.setTextColor(Colors::TextPrimary, Colors::CardBg);
        lcd.setTextSize(1);
        lcd.drawString(timeMini.c_str(), 464, 16);
    }

    // Render individual page content
    switch (_currentPage) {
        case UIPage::PAGE_CLOCK:         drawClockPage(fullRedraw); break;
        case UIPage::PAGE_WEATHER:       drawWeatherPage(fullRedraw); break;
        case UIPage::PAGE_FORECAST:      drawForecastPage(fullRedraw); break;
        case UIPage::PAGE_TASKS:         drawTasksPage(fullRedraw); break;
        case UIPage::PAGE_TIMER:         drawTimerPage(fullRedraw); break;
        case UIPage::PAGE_ALARM:         drawAlarmPage(fullRedraw); break;
        case UIPage::PAGE_DEVICE_STATUS: drawDeviceStatusPage(fullRedraw); break;
        default: break;
    }
}

// ----------------------------------------------------------------------------
// PAGE 1: MAIN DASHBOARD (DIGITAL CLOCK, DATE & TODAY'S WEATHER)
// ----------------------------------------------------------------------------
void UIManager::drawClockPage(bool fullRedraw) {
    DisplayManager& dm = DisplayManager::instance();
    LGFX_NEXORA& lcd = dm.getDisplay();
    TimeManager& tm = TimeManager::instance();
    WeatherManager& wm = WeatherManager::instance();
    AlarmManager& am = AlarmManager::instance();
    TaskManager& tkm = TaskManager::instance();

    int hour = tm.getHour();
    int min = tm.getMinute();
    int sec = tm.getSecond();
    bool isPM = (hour >= 12);
    int displayHour = hour;
    if (!g_config.use24Hour) {
        displayHour = hour % 12;
        if (displayHour == 0) displayHour = 12;
    }

    char timeMainBuf[8];
    snprintf(timeMainBuf, sizeof(timeMainBuf), "%02d:%02d", displayHour, min);

    char secBuf[8];
    snprintf(secBuf, sizeof(secBuf), ":%02d", sec);

    if (fullRedraw) {
        dm.clearContentArea();

        // ====================================================================
        // CARD 1: MAIN CLOCK & DATE HUB (TOP HALF)
        // ====================================================================
        dm.drawCard(12, 38, 456, 132);

        // Location tag
        lcd.setTextDatum(textdatum_t::top_left);
        lcd.setTextColor(Colors::AccentCyan, Colors::CardBg);
        lcd.setTextSize(1);
        lcd.setFont(&fonts::Font0);
        char locTag[48];
        snprintf(locTag, sizeof(locTag), " %s", g_config.locationName);
        lcd.drawString(locTag, 24, 46);

        // Station & NTP status
        lcd.setTextDatum(textdatum_t::top_right);
        lcd.setTextColor(Colors::TextMuted, Colors::CardBg);
        const char* statusTag = tm.isSynced() ? "NTP SYNCED" : "RTC CLOCK";
        lcd.drawString(statusTag, 456, 46);

        // Full Date String
        String dateStr = tm.getDateString();
        lcd.setTextDatum(textdatum_t::middle_center);
        lcd.setTextColor(Colors::TextPrimary, Colors::CardBg);
        lcd.setTextSize(1);
        lcd.setFont(&fonts::Font0);
        lcd.drawString(dateStr.c_str(), 240, 132);

        // Glance status badges (Active Alarm / Pending Tasks)
        char alarmGlance[32] = "No Alarms";
        for (size_t i = 0; i < am.getAlarmCount(); i++) {
            const NEXORAAlarm* a = am.getAlarm(i);
            if (a && a->enabled) {
                snprintf(alarmGlance, sizeof(alarmGlance), "Alarm: %02d:%02d", a->hour, a->minute);
                break;
            }
        }

        size_t pendingTasks = 0;
        for (size_t i = 0; i < tkm.getTaskCount(); i++) {
            const NEXORATask* t = tkm.getTask(i);
            if (t && !t->completed) pendingTasks++;
        }
        char taskGlance[32];
        if (pendingTasks == 0) {
            snprintf(taskGlance, sizeof(taskGlance), "All Tasks Done");
        } else {
            snprintf(taskGlance, sizeof(taskGlance), "%u Tasks Pending", (unsigned)pendingTasks);
        }

        lcd.setTextDatum(textdatum_t::middle_left);
        lcd.setTextColor(Colors::TextMuted, Colors::CardBg);
        lcd.drawString(alarmGlance, 24, 154);

        lcd.setTextDatum(textdatum_t::middle_right);
        lcd.drawString(taskGlance, 456, 154);

        // ====================================================================
        // CARD 2: TODAY'S WEATHER DASHBOARD (BOTTOM HALF)
        // ====================================================================
        dm.drawCard(12, 176, 456, 136, "TODAY'S WEATHER");
        const WeatherData& w = wm.getData();

        // Temperature (Large in Accent Amber)
        char tempBuf[16];
        snprintf(tempBuf, sizeof(tempBuf), "%.1f %s", 
                 g_config.useCelsius ? w.temperature : (w.temperature * 1.8f + 32.0f),
                 g_config.useCelsius ? "C" : "F");
        lcd.setFont(&fonts::Font4);
        lcd.setTextSize(1);
        lcd.setTextColor(Colors::AccentAmber, Colors::CardBg);
        lcd.setTextDatum(textdatum_t::top_left);
        lcd.drawString(tempBuf, 28, 204);

        // Condition text
        lcd.setFont(&fonts::Font0);
        lcd.setTextSize(2);
        lcd.setTextColor(Colors::TextPrimary, Colors::CardBg);
        lcd.drawString(w.condition, 28, 238);

        // Feels like & Forecast High/Low
        char feelsBuf[48];
        snprintf(feelsBuf, sizeof(feelsBuf), "Feels: %.1f%s  |  H: %.0f L: %.0f",
                 g_config.useCelsius ? w.feelsLike : (w.feelsLike * 1.8f + 32.0f),
                 g_config.useCelsius ? "C" : "F",
                 w.forecastCount > 0 ? w.forecast[0].tempMax : 25.0f,
                 w.forecastCount > 0 ? w.forecast[0].tempMin : 16.0f);
        lcd.setTextSize(1);
        lcd.setTextColor(Colors::TextMuted, Colors::CardBg);
        lcd.drawString(feelsBuf, 28, 268);

        char wtrLocBuf[32];
        snprintf(wtrLocBuf, sizeof(wtrLocBuf), "Station: %s", strlen(w.location) > 0 ? w.location : g_config.locationName);
        lcd.drawString(wtrLocBuf, 28, 286);

        // Vertical divider between temperature and metrics
        lcd.drawFastVLine(205, 196, 104, Colors::CardBorder);

        // Right side: 3 Metric Cards
        // 1. Humidity
        lcd.drawRoundRect(216, 196, 74, 72, 4, Colors::CardBorder);
        lcd.setTextDatum(textdatum_t::middle_center);
        lcd.setTextColor(Colors::TextMuted, Colors::CardBg);
        lcd.drawString("HUMIDITY", 253, 208);
        char humBuf[16];
        snprintf(humBuf, sizeof(humBuf), "%d%%", w.humidity);
        lcd.setTextColor(Colors::AccentCyan, Colors::CardBg);
        lcd.setTextSize(2);
        lcd.drawString(humBuf, 253, 232);
        lcd.setTextSize(1);
        lcd.setTextColor(Colors::TextMuted, Colors::CardBg);
        lcd.drawString("RH", 253, 256);

        // 2. Wind
        lcd.drawRoundRect(296, 196, 74, 72, 4, Colors::CardBorder);
        lcd.setTextColor(Colors::TextMuted, Colors::CardBg);
        lcd.drawString("WIND", 333, 208);
        char windBuf[16];
        snprintf(windBuf, sizeof(windBuf), "%.1f", w.windSpeed);
        lcd.setTextColor(Colors::AccentCyan, Colors::CardBg);
        lcd.setTextSize(2);
        lcd.drawString(windBuf, 333, 232);
        lcd.setTextSize(1);
        lcd.setTextColor(Colors::TextMuted, Colors::CardBg);
        lcd.drawString("m/s", 333, 256);

        // 3. Barometer
        lcd.drawRoundRect(376, 196, 82, 72, 4, Colors::CardBorder);
        lcd.setTextColor(Colors::TextMuted, Colors::CardBg);
        lcd.drawString("BAROMETER", 417, 208);
        char pressBuf[16];
        snprintf(pressBuf, sizeof(pressBuf), "%d", w.pressure);
        lcd.setTextColor(Colors::AccentCyan, Colors::CardBg);
        lcd.setTextSize(2);
        lcd.drawString(pressBuf, 417, 232);
        lcd.setTextSize(1);
        lcd.setTextColor(Colors::TextMuted, Colors::CardBg);
        lcd.drawString("hPa", 417, 256);

        // Freshness status
        lcd.setTextDatum(textdatum_t::middle_left);
        lcd.setTextColor(w.isCached ? Colors::AccentAmber : Colors::SuccessGreen, Colors::CardBg);
        String ageText = (w.isCached ? "[Cached] " : "[Live] ") + wm.getAgeString();
        lcd.drawString(ageText.c_str(), 218, 286);
    }

    // ========================================================================
    // DYNAMIC DIGITAL TIME (Updated every second with zero flicker)
    // ========================================================================
    // Clean overwrite of time display bounding box
    lcd.fillRect(145, 64, 195, 54, Colors::CardBg);

    // 1. HH:MM in 7-Segment Digital Clock Font
    lcd.setFont(&fonts::Font7);
    lcd.setTextSize(1);
    lcd.setTextDatum(textdatum_t::top_left);
    lcd.setTextColor(Colors::AccentCyan, Colors::CardBg);
    lcd.drawString(timeMainBuf, 154, 66);

    // 2. Seconds counter (:SS) in clean Font4
    lcd.setFont(&fonts::Font4);
    lcd.setTextSize(1);
    lcd.setTextColor(Colors::AccentAmber, Colors::CardBg);
    lcd.drawString(secBuf, 290, 68);

    // 3. AM/PM or 24H mode indicator below seconds
    lcd.setFont(&fonts::Font0);
    lcd.setTextSize(1);
    lcd.setTextColor(Colors::TextMuted, Colors::CardBg);
    if (!g_config.use24Hour) {
        lcd.drawString(isPM ? "PM" : "AM", 292, 98);
    } else {
        lcd.drawString("24H", 292, 98);
    }

    // Restore standard font and size
    lcd.setFont(&fonts::Font0);
    lcd.setTextSize(1);
}

// ----------------------------------------------------------------------------
// PAGE 2: LIVE WEATHER
// ----------------------------------------------------------------------------
void UIManager::drawWeatherPage(bool fullRedraw) {
    if (!fullRedraw) return;

    DisplayManager& dm = DisplayManager::instance();
    LGFX_NEXORA& lcd = dm.getDisplay();
    dm.clearContentArea();

    WeatherManager& wm = WeatherManager::instance();
    const WeatherData& w = wm.getData();

    // Main Current Temperature Card (Left side)
    dm.drawCard(20, 50, 210, 240, w.location);

    char tempBuf[24];
    snprintf(tempBuf, sizeof(tempBuf), "%.1f %s", 
             g_config.useCelsius ? w.temperature : (w.temperature * 1.8f + 32.0f),
             g_config.useCelsius ? "C" : "F");

    lcd.setTextDatum(textdatum_t::middle_center);
    lcd.setTextColor(Colors::AccentAmber, Colors::CardBg);
    lcd.setTextSize(4);
    lcd.drawString(tempBuf, 125, 120);

    lcd.setTextColor(Colors::TextPrimary, Colors::CardBg);
    lcd.setTextSize(2);
    lcd.drawString(w.condition, 125, 175);

    char feelsBuf[32];
    snprintf(feelsBuf, sizeof(feelsBuf), "Feels: %.1f %s", 
             g_config.useCelsius ? w.feelsLike : (w.feelsLike * 1.8f + 32.0f),
             g_config.useCelsius ? "C" : "F");
    lcd.setTextColor(Colors::TextMuted, Colors::CardBg);
    lcd.setTextSize(1);
    lcd.drawString(feelsBuf, 125, 215);

    // Weather Metrics Grid (Right side)
    int rx = 245;
    dm.drawCard(rx, 50, 215, 65, "HUMIDITY");
    char humBuf[16];
    snprintf(humBuf, sizeof(humBuf), "%d %%", w.humidity);
    lcd.setTextDatum(textdatum_t::middle_left);
    lcd.setTextColor(Colors::AccentCyan, Colors::CardBg);
    lcd.setTextSize(2);
    lcd.drawString(humBuf, rx + 16, 92);

    dm.drawCard(rx, 125, 215, 65, "WIND SPEED");
    char windBuf[24];
    snprintf(windBuf, sizeof(windBuf), "%.1f m/s", w.windSpeed);
    lcd.setTextColor(Colors::AccentCyan, Colors::CardBg);
    lcd.setTextSize(2);
    lcd.drawString(windBuf, rx + 16, 167);

    dm.drawCard(rx, 200, 215, 65, "BAROMETER");
    char pressBuf[24];
    snprintf(pressBuf, sizeof(pressBuf), "%d hPa", w.pressure);
    lcd.setTextColor(Colors::AccentCyan, Colors::CardBg);
    lcd.setTextSize(2);
    lcd.drawString(pressBuf, rx + 16, 242);

    // Footer Age indication
    lcd.setTextDatum(textdatum_t::bottom_center);
    lcd.setTextColor(w.isCached ? Colors::AccentAmber : Colors::TextMuted, Colors::Background);
    lcd.setTextSize(1);
    String ageLabel = (w.isCached ? "[Cached] " : "") + wm.getAgeString();
    lcd.drawString(ageLabel.c_str(), 240, 310);
}

// ----------------------------------------------------------------------------
// PAGE 3: 3-DAY WEATHER FORECAST
// ----------------------------------------------------------------------------
void UIManager::drawForecastPage(bool fullRedraw) {
    if (!fullRedraw) return;

    DisplayManager& dm = DisplayManager::instance();
    LGFX_NEXORA& lcd = dm.getDisplay();
    dm.clearContentArea();

    WeatherManager& wm = WeatherManager::instance();
    const WeatherData& w = wm.getData();

    int cardW = 136;
    int cardH = 240;
    int startX = 22;
    int gap = 16;

    for (int i = 0; i < 3; i++) {
        int x = startX + (i * (cardW + gap));
        const ForecastItem& f = (i < w.forecastCount) ? w.forecast[i] : w.forecast[0];
        const char* dayName = (i < w.forecastCount) ? f.day : (i == 0 ? "Today" : (i == 1 ? "Tomorrow" : "Day after"));
        const char* condStr = (i < w.forecastCount && strlen(f.condition) > 0) ? f.condition : "Partly Cloudy";

        dm.drawCard(x, 50, cardW, cardH, dayName);

        lcd.setTextDatum(textdatum_t::middle_center);
        lcd.setTextColor(Colors::AccentCyan, Colors::CardBg);
        lcd.setTextSize(2);
        lcd.drawString(condStr, x + (cardW / 2), 120);

        char maxBuf[16];
        snprintf(maxBuf, sizeof(maxBuf), "%.0f C", (i < w.forecastCount) ? f.tempMax : 24.0f);
        lcd.setTextColor(Colors::AccentAmber, Colors::CardBg);
        lcd.setTextSize(3);
        lcd.drawString(maxBuf, x + (cardW / 2), 175);

        char minBuf[16];
        snprintf(minBuf, sizeof(minBuf), "Low: %.0f C", (i < w.forecastCount) ? f.tempMin : 16.0f);
        lcd.setTextColor(Colors::TextMuted, Colors::CardBg);
        lcd.setTextSize(1);
        lcd.drawString(minBuf, x + (cardW / 2), 225);
    }
}

// ----------------------------------------------------------------------------
// PAGE 4: PERSONAL TASKS
// ----------------------------------------------------------------------------
void UIManager::drawTasksPage(bool fullRedraw) {
    if (!fullRedraw) return;

    DisplayManager& dm = DisplayManager::instance();
    LGFX_NEXORA& lcd = dm.getDisplay();
    dm.clearContentArea();

    TaskManager& tm = TaskManager::instance();
    size_t count = tm.getTaskCount();

    if (count == 0) {
        dm.drawCard(50, 70, 380, 180, "TASK MANAGER");
        lcd.setTextDatum(textdatum_t::middle_center);
        lcd.setTextColor(Colors::AccentCyan, Colors::CardBg);
        lcd.setTextSize(2);
        lcd.drawString("All Tasks Completed!", 240, 140);
        lcd.setTextColor(Colors::TextMuted, Colors::CardBg);
        lcd.setTextSize(1);
        lcd.drawString("Publish MQTT command to create new tasks", 240, 180);
        return;
    }

    size_t maxDisplay = 4;
    int startY = 46;
    int itemH = 56;

    for (size_t i = 0; i < count && i < maxDisplay; i++) {
        const NEXORATask* t = tm.getTask(i);
        if (!t) break;

        int y = startY + (i * (itemH + 8));
        dm.drawCard(20, y, 440, itemH);

        // Checkbox square
        int cbX = 35;
        int cbY = y + 18;
        lcd.drawRect(cbX, cbY, 20, 20, Colors::CardBorder);
        if (t->completed) {
            lcd.fillRect(cbX + 3, cbY + 3, 14, 14, Colors::SuccessGreen);
        }

        // Title
        lcd.setTextDatum(textdatum_t::middle_left);
        lcd.setTextColor(t->completed ? Colors::TextMuted : Colors::TextPrimary, Colors::CardBg);
        lcd.setTextSize(1);
        lcd.drawString(t->title, 70, y + 28);

        // Priority badge
        lcd.setTextDatum(textdatum_t::middle_right);
        uint16_t priColor = (t->priority >= 3) ? Colors::AlertRed :
                            (t->priority == 2) ? Colors::AccentAmber : Colors::TextMuted;
        const char* priStr = (t->priority >= 3) ? "HIGH" :
                             (t->priority == 2) ? "MED" : "LOW";
        lcd.setTextColor(priColor, Colors::CardBg);
        lcd.drawString(priStr, 445, y + 28);
    }
}

// ----------------------------------------------------------------------------
// PAGE 5: COUNTDOWN TIMER
// ----------------------------------------------------------------------------
void UIManager::drawTimerPage(bool fullRedraw) {
    DisplayManager& dm = DisplayManager::instance();
    LGFX_NEXORA& lcd = dm.getDisplay();
    TimerManager& tm = TimerManager::instance();

    if (fullRedraw) {
        dm.clearContentArea();

        // Large Card for Timer
        dm.drawCard(50, 50, 380, 230, "COUNTDOWN TIMER");

        // Footer info
        char durBuf[32];
        snprintf(durBuf, sizeof(durBuf), "Total Duration: %u min", tm.getTotalDuration() / 60);
        lcd.setTextDatum(textdatum_t::middle_center);
        lcd.setTextColor(Colors::TextMuted, Colors::CardBg);
        lcd.setTextSize(1);
        lcd.drawString(durBuf, 240, 245);
    }

    // Formatted time digits (MM:SS) - Updated every second with background overwrite
    String remStr = tm.getFormattedRemaining();
    lcd.setTextDatum(textdatum_t::middle_center);
    lcd.setTextColor(Colors::AccentCyan, Colors::CardBg);
    lcd.setTextSize(5);
    lcd.drawString(remStr.c_str(), 240, 135);

    // State Badge
    TimerState st = tm.getState();
    uint16_t badgeCol = Colors::TextMuted;
    if (st == TimerState::RUNNING)   badgeCol = Colors::SuccessGreen;
    else if (st == TimerState::PAUSED)    badgeCol = Colors::AccentAmber;
    else if (st == TimerState::COMPLETED) badgeCol = Colors::AlertRed;

    lcd.setTextColor(badgeCol, Colors::CardBg);
    lcd.setTextSize(2);
    lcd.drawString(tm.getStateString(), 240, 195);
}

// ----------------------------------------------------------------------------
// PAGE 6: ALARMS
// ----------------------------------------------------------------------------
void UIManager::drawAlarmPage(bool fullRedraw) {
    if (!fullRedraw) return;

    DisplayManager& dm = DisplayManager::instance();
    LGFX_NEXORA& lcd = dm.getDisplay();
    dm.clearContentArea();

    AlarmManager& am = AlarmManager::instance();
    size_t count = am.getAlarmCount();

    if (count == 0) {
        dm.drawCard(50, 70, 380, 180, "ALARM MANAGER");
        lcd.setTextDatum(textdatum_t::middle_center);
        lcd.setTextColor(Colors::AccentCyan, Colors::CardBg);
        lcd.setTextSize(2);
        lcd.drawString("No Alarms Configured", 240, 140);
        lcd.setTextColor(Colors::TextMuted, Colors::CardBg);
        lcd.setTextSize(1);
        lcd.drawString("Publish MQTT command to set daily alarms", 240, 180);
        return;
    }

    int startY = 48;
    int cardH = 65;
    for (size_t i = 0; i < count && i < 3; i++) {
        const NEXORAAlarm* a = am.getAlarm(i);
        if (!a) break;

        int y = startY + (i * (cardH + 12));
        dm.drawCard(30, y, 420, cardH, a->name);

        char timeBuf[16];
        snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", a->hour, a->minute);

        lcd.setTextDatum(textdatum_t::middle_left);
        lcd.setTextColor(a->enabled ? Colors::TextPrimary : Colors::TextMuted, Colors::CardBg);
        lcd.setTextSize(3);
        lcd.drawString(timeBuf, 45, y + 38);

        // Status switch indicator
        lcd.setTextDatum(textdatum_t::middle_right);
        lcd.setTextColor(a->enabled ? Colors::SuccessGreen : Colors::TextMuted, Colors::CardBg);
        lcd.setTextSize(2);
        lcd.drawString(a->enabled ? "ON" : "OFF", 430, y + 38);
    }
}

// ----------------------------------------------------------------------------
// PAGE 7: DEVICE / NETWORK STATUS
// ----------------------------------------------------------------------------
void UIManager::drawDeviceStatusPage(bool fullRedraw) {
    DisplayManager& dm = DisplayManager::instance();
    LGFX_NEXORA& lcd = dm.getDisplay();
    DeviceManager& dev = DeviceManager::instance();

    int leftW = 210;
    int rightW = 210;

    if (fullRedraw) {
        dm.clearContentArea();

        // Left Card: Device Hardware & System
        dm.drawCard(20, 50, leftW, 240, "DEVICE TELEMETRY");

        lcd.setTextDatum(textdatum_t::top_left);
        lcd.setTextSize(1);

        int y = 80;
        lcd.setTextColor(Colors::TextMuted, Colors::CardBg);
        lcd.drawString("Device ID:", 35, y);
        lcd.setTextColor(Colors::AccentCyan, Colors::CardBg);
        lcd.drawString(g_config.deviceId, 35, y + 14);

        y += 38;
        lcd.setTextColor(Colors::TextMuted, Colors::CardBg);
        lcd.drawString("Firmware Version:", 35, y);
        lcd.setTextColor(Colors::TextPrimary, Colors::CardBg);
        lcd.drawString(NEXORA_FIRMWARE_VERSION, 35, y + 14);

        // Right Card: Connectivity
        dm.drawCard(250, 50, rightW, 240, "CONNECTIVITY");

        WiFiManager& wifiMgr = WiFiManager::instance();
        y = 74;
        lcd.setTextColor(Colors::TextMuted, Colors::CardBg);
        lcd.drawString("Station Wi-Fi:", 265, y);
        lcd.setTextColor(Colors::AccentCyan, Colors::CardBg);
        lcd.drawString(wifiMgr.isConnected() ? wifiMgr.getSSID().c_str() : "Connecting/Offline", 265, y + 13);

        y += 33;
        lcd.setTextColor(Colors::TextMuted, Colors::CardBg);
        lcd.drawString("Station IP:", 265, y);
        lcd.setTextColor(Colors::TextPrimary, Colors::CardBg);
        lcd.drawString(wifiMgr.isConnected() ? wifiMgr.getIP().c_str() : "None", 265, y + 13);

        y += 33;
        lcd.setTextColor(Colors::TextMuted, Colors::CardBg);
        lcd.drawString("Fallback SoftAP:", 265, y);
        lcd.setTextColor(wifiMgr.isAPActive() ? Colors::SuccessGreen : Colors::TextMuted, Colors::CardBg);
        lcd.drawString(wifiMgr.isAPActive() ? wifiMgr.getAPSSID().c_str() : "Inactive", 265, y + 13);

        y += 33;
        lcd.setTextColor(Colors::TextMuted, Colors::CardBg);
        lcd.drawString("AP IP / Web OTA:", 265, y);
        lcd.setTextColor(wifiMgr.isAPActive() ? Colors::AccentAmber : Colors::TextMuted, Colors::CardBg);
        lcd.drawString(wifiMgr.isAPActive() ? "192.168.4.1/update" : "Disabled", 265, y + 13);

        y += 33;
        lcd.setTextColor(Colors::TextMuted, Colors::CardBg);
        lcd.drawString("Wi-Fi RSSI / MQTT:", 265, y);
        char netBuf[32];
        bool online = (dev.getState() == DeviceState::ONLINE);
        if (wifiMgr.isConnected()) {
            snprintf(netBuf, sizeof(netBuf), "%d dBm | %s", wifiMgr.getRSSI(), online ? "MQTT OK" : "No MQTT");
        } else if (wifiMgr.isAPActive()) {
            snprintf(netBuf, sizeof(netBuf), "AP Mode Active");
        } else {
            snprintf(netBuf, sizeof(netBuf), "Offline");
        }
        lcd.setTextColor(online ? Colors::SuccessGreen : (wifiMgr.isAPActive() ? Colors::AccentAmber : Colors::AlertRed), Colors::CardBg);
        lcd.drawString(netBuf, 265, y + 13);
    }

    // Dynamic metrics on Left Card (Updated every second with background overwrite)
    lcd.setTextDatum(textdatum_t::top_left);
    lcd.setTextSize(1);

    int y = 80 + 38 + 38;
    lcd.setTextColor(Colors::TextMuted, Colors::CardBg);
    lcd.drawString("Uptime:                 ", 35, y);
    char upBuf[24];
    snprintf(upBuf, sizeof(upBuf), "%u seconds", dev.getUptimeSeconds());
    lcd.setTextColor(Colors::TextPrimary, Colors::CardBg);
    lcd.drawString(upBuf, 35, y + 14);

    y += 38;
    lcd.setTextColor(Colors::TextMuted, Colors::CardBg);
    lcd.drawString("Free RAM Heap:          ", 35, y);
    char ramBuf[24];
    snprintf(ramBuf, sizeof(ramBuf), "%u KB", dev.getFreeHeap() / 1024);
    lcd.setTextColor(Colors::AccentAmber, Colors::CardBg);
    lcd.drawString(ramBuf, 35, y + 14);
}
