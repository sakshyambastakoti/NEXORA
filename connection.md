# Arduino Nano ESP32 to 3.5" ILI9488 TFT LCD Connection Guide

Pinout mapping for connecting the **Arduino Nano ESP32 (ESP32-S3)** to a **3.5" ILI9488 TFT LCD**.

---

## 1. Primary Setup: 8-Bit Parallel Mode (Active Firmware)

Used by `src/nanoesp.cpp` and `Nano_ESP32_ILI9488_Parallel_Web_Display.ino`.

### Power Connections
| Display Pin | Nano ESP32 Pin | Details |
| :--- | :--- | :--- |
| **VCC / 5V** | **VBUS / 5V** (or 3.3V) | Main display supply |
| **GND** | **GND** | Ground (Common GND) |

### Control Bus
| Display Pin | Nano ESP32 Silk Pin | ESP32-S3 Native GPIO | Function |
| :--- | :--- | :--- | :--- |
| **LCD_RD** | **A0** | `GPIO 1` | Read Strobe |
| **LCD_WR** | **A1** | `GPIO 2` | Write Strobe |
| **LCD_RS / DC** | **A2** | `GPIO 3` | Command / Data Select |
| **LCD_CS** | **A3** | `GPIO 4` | Chip Select (Active LOW) |
| **LCD_RST** | **A4** | `GPIO 11` | Hardware Reset (Active LOW) |

### 8-Bit Parallel Data Bus (D0 – D7)
| Display Pin | Nano ESP32 Silk Pin | ESP32-S3 Native GPIO | Bus Bit |
| :--- | :--- | :--- | :--- |
| **LCD_D0** | **D2** | `GPIO 5` | Bit 0 |
| **LCD_D1** | **D3** | `GPIO 6` | Bit 1 |
| **LCD_D2** | **D4** | `GPIO 7` | Bit 2 |
| **LCD_D3** | **D5** | `GPIO 8` | Bit 3 |
| **LCD_D4** | **D6** | `GPIO 9` | Bit 4 |
| **LCD_D5** | **D7** | `GPIO 10` | Bit 5 |
| **LCD_D6** | **D8** | `GPIO 17` | Bit 6 |
| **LCD_D7** | **D9** | `GPIO 18` | Bit 7 |

---

## 2. Alternative Setup: SPI Mode

Used by SPI-based drivers (such as `Nano_ESP32_ILI9488_Web_Display.ino` with LovyanGFX).

| Display Pin | Nano ESP32 Silk Pin | ESP32-S3 Native GPIO | Function |
| :--- | :--- | :--- | :--- |
| **VCC** | **VBUS / 5V** | — | Power Supply |
| **GND** | **GND** | — | Ground |
| **CS** | **D10** | `GPIO 21` | SPI Chip Select |
| **DC / RS** | **D9** | `GPIO 18` | Data / Command |
| **RST** | **D8** | `GPIO 17` | Hardware Reset |
| **BL / LED** | **D7** | `GPIO 10` | Backlight Control (PWM/Digital) |
| **MOSI / SDI**| **D11** | `GPIO 38` | SPI Data In |
| **MISO / SDO**| **D12** | `GPIO 47` | SPI Data Out |
| **SCK / CLK** | **D13** | `GPIO 48` | SPI Clock |

---

## 3. Configuration Notes
- In PlatformIO, build flag `-D BOARD_USES_HW_GPIO_NUMBERS` is enabled so GPIO numbers in the code correspond directly to ESP32-S3 hardware GPIOs.
- Wire directly using the **Silk Screen Pins** printed on the Arduino Nano ESP32 board for easy physical wiring.
