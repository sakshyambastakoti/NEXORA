#pragma once

#include <Arduino.h>

/**
 * @file HardwareConfig.h
 * @brief Central Hardware Pin Configuration for NEXORA.
 *
 * All pin mappings STRICTLY correspond to connection.md.
 * With -D BOARD_USES_HW_GPIO_NUMBERS enabled in platformio.ini,
 * all GPIO constants correspond directly to ESP32-S3 hardware GPIO numbers.
 */

namespace Hardware {

// ============================================================================
// 1. PRIMARY SETUP: 8-BIT PARALLEL MODE (ACTIVE FIRMWARE)
// ============================================================================

// Control Bus
constexpr int8_t LCD_RD   = 1;   // Nano Silk: A0  -> Read Strobe
constexpr int8_t LCD_WR   = 2;   // Nano Silk: A1  -> Write Strobe
constexpr int8_t LCD_RS   = 3;   // Nano Silk: A2  -> Command/Data Select (DC)
constexpr int8_t LCD_CS   = 4;   // Nano Silk: A3  -> Chip Select (Active LOW)
constexpr int8_t LCD_RST  = 11;  // Nano Silk: A4  -> Hardware Reset (Active LOW)

// 8-Bit Parallel Data Bus (D0 – D7)
constexpr int8_t LCD_D0   = 5;   // Nano Silk: D2  -> Bus Bit 0
constexpr int8_t LCD_D1   = 6;   // Nano Silk: D3  -> Bus Bit 1
constexpr int8_t LCD_D2   = 7;   // Nano Silk: D4  -> Bus Bit 2
constexpr int8_t LCD_D3   = 8;   // Nano Silk: D5  -> Bus Bit 3
constexpr int8_t LCD_D4   = 9;   // Nano Silk: D6  -> Bus Bit 4
constexpr int8_t LCD_D5   = 10;  // Nano Silk: D7  -> Bus Bit 5
constexpr int8_t LCD_D6   = 17;  // Nano Silk: D8  -> Bus Bit 6
constexpr int8_t LCD_D7   = 18;  // Nano Silk: D9  -> Bus Bit 7

// ============================================================================
// 2. ALTERNATIVE SETUP: SPI MODE (REFERENCE / CONDITIONAL)
// ============================================================================
namespace SPI_Alternative {
    constexpr int8_t SPI_CS   = 21;  // Nano Silk: D10 -> SPI Chip Select
    constexpr int8_t SPI_DC   = 18;  // Nano Silk: D9  -> Data / Command
    constexpr int8_t SPI_RST  = 17;  // Nano Silk: D8  -> Hardware Reset
    constexpr int8_t SPI_BL   = 10;  // Nano Silk: D7  -> Backlight Control
    constexpr int8_t SPI_MOSI = 38;  // Nano Silk: D11 -> SPI Data In
    constexpr int8_t SPI_MISO = 47;  // Nano Silk: D12 -> SPI Data Out
    constexpr int8_t SPI_SCK  = 48;  // Nano Silk: D13 -> SPI Clock
}

// Display specifications
constexpr uint16_t SCREEN_WIDTH  = 480;
constexpr uint16_t SCREEN_HEIGHT = 320;
constexpr uint8_t  SCREEN_ROTATION = 1; // Landscape (480x320)

} // namespace Hardware
