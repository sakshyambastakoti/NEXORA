#include "DisplayManager.h"

DisplayManager& DisplayManager::instance() {
    static DisplayManager inst;
    return inst;
}

bool DisplayManager::begin() {
    Serial.println(F("[DISPLAY] Initializing 3.5\" ILI9488 TFT LCD in 8-Bit Parallel Mode..."));
    _lcd.init();
    _lcd.setRotation(Hardware::SCREEN_ROTATION);
    _lcd.setColorDepth(16);
    _lcd.fillScreen(Colors::Background);
    _initialized = true;
    Serial.println(F("[DISPLAY] ILI9488 initialized successfully."));
    return true;
}

void DisplayManager::update() {
    // Reserved for display refresh / backlight management
}

void DisplayManager::showBootScreen(const char* statusMsg, int progressPercent) {
    if (!_initialized) begin();

    // Clamp progress
    if (progressPercent < 0) progressPercent = 0;
    if (progressPercent > 100) progressPercent = 100;

    // Draw main frame background
    _lcd.fillScreen(Colors::Background);

    // Subtle accent line at the very top
    _lcd.fillRect(0, 0, 480, 3, Colors::AccentCyan);

    // Title & Logo
    _lcd.setTextDatum(textdatum_t::middle_center);
    _lcd.setTextColor(Colors::AccentCyan, Colors::Background);
    _lcd.setTextSize(4);
    _lcd.drawString("N E X O R A", 240, 80);

    _lcd.setTextColor(Colors::TextMuted, Colors::Background);
    _lcd.setTextSize(1);
    _lcd.drawString("SMART PERSONAL INFORMATION STATION", 240, 120);

    // Progress Bar Outer Frame
    int barX = 80;
    int barY = 180;
    int barW = 320;
    int barH = 16;
    _lcd.drawRoundRect(barX - 2, barY - 2, barW + 4, barH + 4, 4, Colors::CardBorder);
    _lcd.fillRoundRect(barX, barY, barW, barH, 2, Colors::CardBg);

    // Progress Bar Fill
    int fillW = (barW * progressPercent) / 100;
    if (fillW > 0) {
        _lcd.fillRoundRect(barX, barY, fillW, barH, 2, Colors::AccentCyan);
    }

    // Status message and percentage
    _lcd.setTextDatum(textdatum_t::middle_center);
    _lcd.setTextColor(Colors::TextPrimary, Colors::Background);
    _lcd.setTextSize(1);
    _lcd.drawString(statusMsg, 240, 220);

    char pctStr[16];
    snprintf(pctStr, sizeof(pctStr), "%d%%", progressPercent);
    _lcd.setTextColor(Colors::AccentAmber, Colors::Background);
    _lcd.drawString(pctStr, 240, 245);

    // Footer info
    _lcd.setTextColor(Colors::TextMuted, Colors::Background);
    _lcd.setTextDatum(textdatum_t::bottom_center);
    _lcd.drawString("Firmware v1.0.0 | Arduino Nano ESP32", 240, 310);
}

void DisplayManager::renderHeader(const char* pageTitle, UIPage currentPage, int8_t rssi, bool mqttOk, const char* timeStr) {
    // Header background bar
    _lcd.fillRect(0, 0, 480, 32, Colors::CardBg);
    _lcd.drawFastHLine(0, 32, 480, Colors::CardBorder);

    // Page Title on the left
    _lcd.setTextDatum(textdatum_t::middle_left);
    _lcd.setTextColor(Colors::AccentCyan, Colors::CardBg);
    _lcd.setTextSize(2);
    _lcd.drawString(pageTitle, 16, 16);

    // Mini Page Indicator Dots in the center
    int totalPages = static_cast<int>(UIPage::PAGE_COUNT);
    int startDotX = 240 - ((totalPages * 14) / 2);
    for (int i = 0; i < totalPages; i++) {
        int dotX = startDotX + (i * 14);
        if (i == static_cast<int>(currentPage)) {
            _lcd.fillCircle(dotX, 16, 4, Colors::AccentCyan);
        } else {
            _lcd.drawCircle(dotX, 16, 3, Colors::CardBorder);
        }
    }

    // Right Side: Wi-Fi RSSI indicator bars, MQTT Status, and Clock
    int rightX = 464;

    // Mini clock
    _lcd.setTextDatum(textdatum_t::middle_right);
    _lcd.setTextColor(Colors::TextPrimary, Colors::CardBg);
    _lcd.setTextSize(1);
    _lcd.drawString(timeStr, rightX, 16);
    rightX -= 48;

    // MQTT status indicator
    _lcd.fillCircle(rightX, 16, 4, mqttOk ? Colors::SuccessGreen : Colors::AlertRed);
    rightX -= 14;

    // Wi-Fi signal icon (3 mini bars)
    int wifiBars = 0;
    if (rssi > -60) wifiBars = 3;
    else if (rssi > -75) wifiBars = 2;
    else if (rssi > -90) wifiBars = 1;

    for (int b = 0; b < 3; b++) {
        int barH = 4 + (b * 3);
        uint16_t bColor = (b < wifiBars) ? Colors::SuccessGreen : Colors::CardBorder;
        _lcd.fillRect(rightX - 12 + (b * 4), 22 - barH, 3, barH, bColor);
    }
}

void DisplayManager::drawCard(int x, int y, int w, int h, const char* label) {
    _lcd.fillRoundRect(x, y, w, h, 6, Colors::CardBg);
    _lcd.drawRoundRect(x, y, w, h, 6, Colors::CardBorder);
    if (label != nullptr) {
        _lcd.setTextDatum(textdatum_t::top_left);
        _lcd.setTextColor(Colors::AccentCyan, Colors::CardBg);
        _lcd.setTextSize(1);
        _lcd.drawString(label, x + 10, y + 8);
    }
}

void DisplayManager::clearContentArea() {
    _lcd.fillRect(0, 33, 480, 287, Colors::Background);
}
