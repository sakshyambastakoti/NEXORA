#include "UIManager.h"
#include "TimeManager.h"
#include "WeatherManager.h"
#include "TaskManager.h"
#include "TimerManager.h"
#include "AlarmManager.h"
#include "DeviceManager.h"
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
    // 1. Auto-rotation handling
    if (g_config.autoRotate && g_config.pageIntervalMs > 0) {
        if (millis() - _lastPageSwitchMs >= g_config.pageIntervalMs) {
            nextPage();
        }
    }

    // 2. Per-second refresh for clock and timer
    if (millis() - _lastSecondUpdateMs >= 1000 || _needsFullRedraw) {
        _lastSecondUpdateMs = millis();
        drawCurrentPage();
        _needsFullRedraw = false;
    }
}

void UIManager::drawCurrentPage() {
    DisplayManager& dm = DisplayManager::instance();
    LGFX_NEXORA& lcd = dm.getDisplay();

    // WiFi and status info
    int8_t rssi = WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : -100;
    bool mqttOk = (DeviceManager::instance().getState() == DeviceState::ONLINE);
    String timeMini = TimeManager::instance().getTimeString(false, g_config.use24Hour);

    const char* title = "NEXORA";
    switch (_currentPage) {
        case UIPage::PAGE_CLOCK:         title = "CLOCK & DATE"; break;
        case UIPage::PAGE_WEATHER:       title = "WEATHER"; break;
        case UIPage::PAGE_FORECAST:      title = "3-DAY FORECAST"; break;
        case UIPage::PAGE_TASKS:         title = "MY TASKS"; break;
        case UIPage::PAGE_TIMER:         title = "TIMER"; break;
        case UIPage::PAGE_ALARM:         title = "SCHEDULED ALARMS"; break;
        case UIPage::PAGE_DEVICE_STATUS: title = "SYSTEM STATUS"; break;
        default:                         title = "NEXORA"; break;
    }

    // Render universal header
    dm.renderHeader(title, _currentPage, rssi, mqttOk, timeMini.c_str());

    // Render individual page content
    switch (_currentPage) {
        case UIPage::PAGE_CLOCK:         drawClockPage(); break;
        case UIPage::PAGE_WEATHER:       drawWeatherPage(); break;
        case UIPage::PAGE_FORECAST:      drawForecastPage(); break;
        case UIPage::PAGE_TASKS:         drawTasksPage(); break;
        case UIPage::PAGE_TIMER:         drawTimerPage(); break;
        case UIPage::PAGE_ALARM:         drawAlarmPage(); break;
        case UIPage::PAGE_DEVICE_STATUS: drawDeviceStatusPage(); break;
        default: break;
    }
}

// ----------------------------------------------------------------------------
// PAGE 1: CLOCK & DATE
// ----------------------------------------------------------------------------
void UIManager::drawClockPage() {
    DisplayManager& dm = DisplayManager::instance();
    LGFX_NEXORA& lcd = dm.getDisplay();
    dm.clearContentArea();

    TimeManager& tm = TimeManager::instance();
    WeatherManager& wm = WeatherManager::instance();

    // 1. Giant Digital Time
    String timeStr = tm.getTimeString(true, g_config.use24Hour);
    lcd.setTextDatum(textdatum_t::middle_center);
    lcd.setTextColor(Colors::AccentCyan, Colors::Background);
    lcd.setTextSize(5);
    lcd.drawString(timeStr.c_str(), 240, 100);

    // 2. Full Date & Day
    String dateStr = tm.getDateString();
    lcd.setTextColor(Colors::TextPrimary, Colors::Background);
    lcd.setTextSize(2);
    lcd.drawString(dateStr.c_str(), 240, 160);

    // 3. Location & Timezone Card
    dm.drawCard(40, 195, 190, 95, "LOCATION");
    lcd.setTextDatum(textdatum_t::middle_center);
    lcd.setTextColor(Colors::TextPrimary, Colors::CardBg);
    lcd.setTextSize(2);
    lcd.drawString(g_config.locationName, 135, 235);
    lcd.setTextColor(Colors::TextMuted, Colors::CardBg);
    lcd.setTextSize(1);
    lcd.drawString(g_config.timezone, 135, 265);

    // 4. Weather Glance Card
    dm.drawCard(250, 195, 190, 95, "WEATHER BRIEF");
    const WeatherData& w = wm.getData();
    char tempBuf[32];
    snprintf(tempBuf, sizeof(tempBuf), "%.1f %s", 
             g_config.useCelsius ? w.temperature : (w.temperature * 1.8f + 32.0f),
             g_config.useCelsius ? "°C" : "°F");
    lcd.setTextDatum(textdatum_t::middle_center);
    lcd.setTextColor(Colors::AccentAmber, Colors::CardBg);
    lcd.setTextSize(2);
    lcd.drawString(tempBuf, 345, 235);

    lcd.setTextColor(Colors::TextMuted, Colors::CardBg);
    lcd.setTextSize(1);
    lcd.drawString(w.condition, 345, 265);
}

// ----------------------------------------------------------------------------
// PAGE 2: LIVE WEATHER
// ----------------------------------------------------------------------------
void UIManager::drawWeatherPage() {
    DisplayManager& dm = DisplayManager::instance();
    LGFX_NEXORA& lcd = dm.getDisplay();
    dm.clearContentArea();

    WeatherManager& wm = WeatherManager::instance();
    const WeatherData& w = wm.getData();

    // Main Current Temperature Card (Left side)
    dm.drawCard(20, 50, 210, 240, w.location);

    char tempBuf[24];
    snprintf(tempBuf, sizeof(tempBuf), "%.1f°%s", 
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
    snprintf(feelsBuf, sizeof(feelsBuf), "Feels like: %.1f°%s", 
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
void UIManager::drawForecastPage() {
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

        dm.drawCard(x, 50, cardW, cardH, dayName);

        lcd.setTextDatum(textdatum_t::middle_center);
        lcd.setTextColor(Colors::AccentCyan, Colors::CardBg);
        lcd.setTextSize(2);
        lcd.drawString(f.condition, x + (cardW / 2), 120);

        char maxBuf[16];
        snprintf(maxBuf, sizeof(maxBuf), "%.0f°", f.tempMax);
        lcd.setTextColor(Colors::AccentAmber, Colors::CardBg);
        lcd.setTextSize(3);
        lcd.drawString(maxBuf, x + (cardW / 2), 175);

        char minBuf[16];
        snprintf(minBuf, sizeof(minBuf), "Low: %.0f°", f.tempMin);
        lcd.setTextColor(Colors::TextMuted, Colors::CardBg);
        lcd.setTextSize(1);
        lcd.drawString(minBuf, x + (cardW / 2), 225);
    }
}

// ----------------------------------------------------------------------------
// PAGE 4: PERSONAL TASKS
// ----------------------------------------------------------------------------
void UIManager::drawTasksPage() {
    DisplayManager& dm = DisplayManager::instance();
    LGFX_NEXORA& lcd = dm.getDisplay();
    dm.clearContentArea();

    TaskManager& tm = TaskManager::instance();
    size_t count = tm.getTaskCount();

    if (count == 0) {
        lcd.setTextDatum(textdatum_t::middle_center);
        lcd.setTextColor(Colors::TextMuted, Colors::Background);
        lcd.setTextSize(2);
        lcd.drawString("All tasks completed!", 240, 160);
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
void UIManager::drawTimerPage() {
    DisplayManager& dm = DisplayManager::instance();
    LGFX_NEXORA& lcd = dm.getDisplay();
    dm.clearContentArea();

    TimerManager& tm = TimerManager::instance();

    // Large Card for Timer
    dm.drawCard(50, 50, 380, 230, "COUNTDOWN TIMER");

    // Formatted time digits (MM:SS)
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

    // Footer info
    char durBuf[32];
    snprintf(durBuf, sizeof(durBuf), "Total Duration: %u min", tm.getTotalDuration() / 60);
    lcd.setTextColor(Colors::TextMuted, Colors::CardBg);
    lcd.setTextSize(1);
    lcd.drawString(durBuf, 240, 245);
}

// ----------------------------------------------------------------------------
// PAGE 6: ALARMS
// ----------------------------------------------------------------------------
void UIManager::drawAlarmPage() {
    DisplayManager& dm = DisplayManager::instance();
    LGFX_NEXORA& lcd = dm.getDisplay();
    dm.clearContentArea();

    AlarmManager& am = AlarmManager::instance();
    size_t count = am.getAlarmCount();

    if (count == 0) {
        lcd.setTextDatum(textdatum_t::middle_center);
        lcd.setTextColor(Colors::TextMuted, Colors::Background);
        lcd.setTextSize(2);
        lcd.drawString("No alarms configured", 240, 160);
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
void UIManager::drawDeviceStatusPage() {
    DisplayManager& dm = DisplayManager::instance();
    LGFX_NEXORA& lcd = dm.getDisplay();
    dm.clearContentArea();

    DeviceManager& dev = DeviceManager::instance();

    int leftW = 210;
    int rightW = 210;

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

    y += 38;
    lcd.setTextColor(Colors::TextMuted, Colors::CardBg);
    lcd.drawString("Uptime:", 35, y);
    char upBuf[24];
    snprintf(upBuf, sizeof(upBuf), "%u seconds", dev.getUptimeSeconds());
    lcd.setTextColor(Colors::TextPrimary, Colors::CardBg);
    lcd.drawString(upBuf, 35, y + 14);

    y += 38;
    lcd.setTextColor(Colors::TextMuted, Colors::CardBg);
    lcd.drawString("Free RAM Heap:", 35, y);
    char ramBuf[24];
    snprintf(ramBuf, sizeof(ramBuf), "%u KB", dev.getFreeHeap() / 1024);
    lcd.setTextColor(Colors::AccentAmber, Colors::CardBg);
    lcd.drawString(ramBuf, 35, y + 14);

    // Right Card: Connectivity
    dm.drawCard(250, 50, rightW, 240, "CONNECTIVITY");

    y = 80;
    lcd.setTextColor(Colors::TextMuted, Colors::CardBg);
    lcd.drawString("Wi-Fi Network:", 265, y);
    lcd.setTextColor(Colors::AccentCyan, Colors::CardBg);
    lcd.drawString(WiFi.status() == WL_CONNECTED ? WiFi.SSID().c_str() : "Disconnected", 265, y + 14);

    y += 38;
    lcd.setTextColor(Colors::TextMuted, Colors::CardBg);
    lcd.drawString("IP Address:", 265, y);
    lcd.setTextColor(Colors::TextPrimary, Colors::CardBg);
    lcd.drawString(WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString().c_str() : "None", 265, y + 14);

    y += 38;
    lcd.setTextColor(Colors::TextMuted, Colors::CardBg);
    lcd.drawString("Wi-Fi RSSI:", 265, y);
    char rssiBuf[24];
    snprintf(rssiBuf, sizeof(rssiBuf), "%d dBm", WiFi.RSSI());
    lcd.setTextColor(Colors::TextPrimary, Colors::CardBg);
    lcd.drawString(WiFi.status() == WL_CONNECTED ? rssiBuf : "N/A", 265, y + 14);

    y += 38;
    lcd.setTextColor(Colors::TextMuted, Colors::CardBg);
    lcd.drawString("MQTT Broker:", 265, y);
    bool online = (dev.getState() == DeviceState::ONLINE);
    lcd.setTextColor(online ? Colors::SuccessGreen : Colors::AlertRed, Colors::CardBg);
    lcd.drawString(online ? "Connected" : "Disconnected", 265, y + 14);
}
