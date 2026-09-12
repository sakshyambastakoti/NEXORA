# NEXORA — Smart Personal Information Station

**NEXORA** is a connected desktop smart information station powered by an **Arduino Nano ESP32 (ESP32-S3)** and an **ILI9488 3.5" TFT LCD** running on an **8-bit parallel bus**.

Designed with commercial-grade firmware architecture, NEXORA is offline-capable, remotely configurable via MQTT, and provides a sleek multi-page dashboard.

---

## Key Features

- **High-Speed Parallel Display Engine**: Driven by LovyanGFX on an 8-bit parallel bus for fast, flicker-free rendering on the 3.5" 480x320 screen.
- **7 Dynamic Display Pages**:
  1. **Clock & Calendar**: Large digital typography, 12h/24h modes, date, day of week, and location.
  2. **Live Weather**: Current temperature, feels-like, humidity, barometer, wind speed, condition, and update freshness.
  3. **3-Day Forecast**: Multi-day weather projections with highs, lows, and icons.
  4. **My Tasks**: Persistent to-do list with priority badges and completion states.
  5. **Countdown Timer**: Non-blocking timer with start, pause, resume, and completion triggers.
  6. **Scheduled Alarms**: Recurring alarms with day-of-week repeat masks.
  7. **System & Network Status**: IP address, Wi-Fi RSSI, broker connection state, free RAM, and uptime.
- **Offline-First Resilience**: Full local timekeeping, task storage, alarms, and cached weather remain completely operational when disconnected from Wi-Fi.
- **Remote MQTT Dashboard Support**: Seamless integration with web dashboards (e.g. hosted on Vercel), featuring a command dispatcher with request acknowledgements.
- **Safe OTA Updates**: Over-The-Air firmware updates with live visual progress feedback on the display.
- **Persistent Storage**: Uses ESP32 NVS Preferences for settings, alarms, tasks, and weather cache with zero flash wear during normal operation.

---

## Hardware Pinout (8-Bit Parallel Mode)

All GPIO definitions are defined in [include/HardwareConfig.h](include/HardwareConfig.h) strictly following [connection.md](connection.md):

| Display Pin | Nano ESP32 Silk Pin | ESP32-S3 GPIO | Function |
| :--- | :--- | :--- | :--- |
| **VCC / 5V** | **VBUS / 5V** | — | 5V Power Supply |
| **GND** | **GND** | — | Common Ground |
| **LCD_RD** | **A0** | `GPIO 1` | Read Strobe |
| **LCD_WR** | **A1** | `GPIO 2` | Write Strobe |
| **LCD_RS / DC** | **A2** | `GPIO 3` | Command / Data Select |
| **LCD_CS** | **A3** | `GPIO 4` | Chip Select (Active LOW) |
| **LCD_RST** | **A4** | `GPIO 11` | Hardware Reset (Active LOW) |
| **LCD_D0** | **D2** | `GPIO 5` | Data Bit 0 |
| **LCD_D1** | **D3** | `GPIO 6` | Data Bit 1 |
| **LCD_D2** | **D4** | `GPIO 7` | Data Bit 2 |
| **LCD_D3** | **D5** | `GPIO 8` | Data Bit 3 |
| **LCD_D4** | **D6** | `GPIO 9` | Data Bit 4 |
| **LCD_D5** | **D7** | `GPIO 10` | Data Bit 5 |
| **LCD_D6** | **D8** | `GPIO 17` | Data Bit 6 |
| **LCD_D7** | **D9** | `GPIO 18` | Data Bit 7 |

---

## Directory Structure

```text
NEXORA/
├── platformio.ini                  # PlatformIO configuration
├── connection.md                   # Single source of truth for pinout
├── README.md                       # Product documentation
├── .gitignore                      # Git ignore rules
├── include/
│   ├── Config.h                    # System configuration & data structs
│   ├── HardwareConfig.h            # Exact GPIO mapping from connection.md
│   ├── secrets.h.template          # Wi-Fi / MQTT credentials template
│   ├── secrets.h                   # Local credentials (ignored from git)
│   ├── StorageManager.h            # NVS persistent preferences
│   ├── DeviceManager.h             # Device state machine & telemetry
│   ├── DisplayManager.h            # LovyanGFX ILI9488 8-bit parallel setup
│   ├── UIManager.h                 # Pages, carousel auto-rotation, cards
│   ├── WiFiManager.h               # Non-blocking Wi-Fi lifecycle
│   ├── TimeManager.h               # NTP sync & timezone handling
│   ├── WeatherManager.h            # Weather ingestion & offline caching
│   ├── TaskManager.h               # Persistent tasks manager
│   ├── TimerManager.h              # Millis countdown timer
│   ├── AlarmManager.h              # Multi-alarm scheduler
│   ├── MQTTManager.h               # Topics, command dispatcher & ACK
│   └── OTAManager.h                # ArduinoOTA firmware updates
├── src/
│   ├── main.cpp                    # Boot sequence & cooperative scheduler
│   ├── StorageManager.cpp
│   ├── DeviceManager.cpp
│   ├── DisplayManager.cpp
│   ├── UIManager.cpp
│   ├── WiFiManager.cpp
│   ├── TimeManager.cpp
│   ├── WeatherManager.cpp
│   ├── TaskManager.cpp
│   ├── TimerManager.cpp
│   ├── AlarmManager.cpp
│   ├── MQTTManager.cpp
│   └── OTAManager.cpp
└── docs/
    ├── architecture.md             # Subsystem & data flow documentation
    ├── mqtt.md                     # MQTT topics and JSON payload contracts
    └── firmware.md                 # Flashing, troubleshooting, and verification
```

---

## Getting Started

1. **Clone & Open**: Open the repository folder in PlatformIO (VS Code or CLI).
2. **Configure Credentials**:
   ```bash
   cp include/secrets.h.template include/secrets.h
   ```
   Set your Wi-Fi credentials and MQTT broker in `include/secrets.h`.
3. **Build**:
   ```bash
   pio run
   ```
4. **Flash**:
   ```bash
   pio run --target upload
   ```
5. **Monitor**:
   ```bash
   pio device monitor -b 115200
   ```

---

## Detailed Documentation
- [System Architecture](docs/architecture.md)
- [MQTT Protocol & Payload Specification](docs/mqtt.md)
- [Firmware Build & Flashing Guide](docs/firmware.md)
