#pragma once

#include <Arduino.h>
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include "HardwareConfig.h"
#include "Config.h"

// ============================================================================
// LOVYANGFX DISPLAY CLASS FOR ILI9488 8-BIT PARALLEL BUS ON ESP32-S3
// ============================================================================
class LGFX_NEXORA : public lgfx::LGFX_Device {
    lgfx::Panel_ILI9488 _panel_instance;
    lgfx::Bus_Parallel8 _bus_instance;

public:
    LGFX_NEXORA() {
        {
            auto cfg = _bus_instance.config();
            cfg.port = 0;
            cfg.freq_write = 10000000; // 10MHz stable parallel write strobe for ILI9488
            cfg.pin_wr = Hardware::LCD_WR;  // GPIO 2
            cfg.pin_rd = Hardware::LCD_RD;  // GPIO 1
            cfg.pin_rs = Hardware::LCD_RS;  // GPIO 3
            cfg.pin_d0 = Hardware::LCD_D0;  // GPIO 5
            cfg.pin_d1 = Hardware::LCD_D1;  // GPIO 6
            cfg.pin_d2 = Hardware::LCD_D2;  // GPIO 7
            cfg.pin_d3 = Hardware::LCD_D3;  // GPIO 8
            cfg.pin_d4 = Hardware::LCD_D4;  // GPIO 9
            cfg.pin_d5 = Hardware::LCD_D5;  // GPIO 10
            cfg.pin_d6 = Hardware::LCD_D6;  // GPIO 17
            cfg.pin_d7 = Hardware::LCD_D7;  // GPIO 18
            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }
        {
            auto cfg = _panel_instance.config();
            cfg.pin_cs = Hardware::LCD_CS;   // GPIO 4
            cfg.pin_rst = Hardware::LCD_RST; // GPIO 11
            cfg.pin_busy = -1;
            cfg.panel_width = 320;
            cfg.panel_height = 480;
            cfg.offset_x = 0;
            cfg.offset_y = 0;
            cfg.offset_rotation = 0;
            cfg.dummy_read_pixel = 8;
            cfg.dummy_read_bits = 1;
            cfg.readable = false;
            cfg.invert = false;
            cfg.rgb_order = false;
            cfg.dlen_16bit = false;
            cfg.bus_shared = false;
            _panel_instance.config(cfg);
        }
        setPanel(&_panel_instance);
    }
};

// High-Contrast Modern Color Palette (RGB565)
namespace Colors {
    constexpr uint16_t Background    = 0x0000; // Pitch black for high contrast
    constexpr uint16_t CardBg        = 0x10A2; // Dark slate card #101418
    constexpr uint16_t CardBorder    = 0x03EF; // Crisp cyan border #007D7B
    constexpr uint16_t AccentCyan    = 0x07FF; // Ultra-vibrant neon cyan #00FFFF
    constexpr uint16_t AccentAmber   = 0xFDE0; // Warm bright gold #FFA800
    constexpr uint16_t TextPrimary   = 0xFFFF; // Crisp white #FFFFFF
    constexpr uint16_t TextMuted     = 0xCE79; // Silver gray #CCCCCC
    constexpr uint16_t SuccessGreen  = 0x07E0; // Vibrant emerald green #00FF00
    constexpr uint16_t AlertRed      = 0xF800; // Vibrant red #FF0000
}

class DisplayManager {
public:
    static DisplayManager& instance();

    bool begin();
    void update();

    LGFX_NEXORA& getDisplay() { return _lcd; }

    // Screen helpers
    void showBootScreen(const char* statusMsg, int progressPercent);
    void renderHeader(const char* pageTitle, UIPage currentPage, int8_t rssi, bool mqttOk, const char* timeStr);
    void drawCard(int x, int y, int w, int h, const char* label = nullptr);
    void clearContentArea();

private:
    DisplayManager() = default;
    ~DisplayManager() = default;
    DisplayManager(const DisplayManager&) = delete;
    DisplayManager& operator=(const DisplayManager&) = delete;

    LGFX_NEXORA _lcd;
    bool _initialized = false;
};
