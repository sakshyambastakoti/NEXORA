# NEXORA System Architecture

## Overview
**NEXORA** is a modular, high-reliability desktop information station firmware built for the **Arduino Nano ESP32 (ESP32-S3)** paired with a **3.5" ILI9488 TFT LCD** running in **8-bit parallel bus mode**.

---

## Architectural Layers

```
┌────────────────────────────────────────────────────────┐
│                   NEXORA Web Dashboard                 │
│              (Vercel / React / WebSocket)              │
└──────────────────────────┬─────────────────────────────┘
                           │ MQTT JSON over TLS / TCP
┌──────────────────────────▼─────────────────────────────┐
│                    Managed MQTT Broker                 │
│                 (HiveMQ Cloud / Mosquitto)             │
└──────────────────────────┬─────────────────────────────┘
                           │ WiFi
┌──────────────────────────▼─────────────────────────────┐
│                      NEXORA Firmware                   │
│                                                        │
│  ┌──────────────────────────────────────────────────┐  │
│  │               Cooperative Main Loop              │  │
│  └──────┬───────────────┬─────────────────┬─────────┘  │
│         │               │                 │            │
│  ┌──────▼──────┐ ┌──────▼──────┐   ┌──────▼──────┐     │
│  │ UIManager   │ │ MQTTManager │   │ TimeManager │     │
│  │ (7 UI Pages)│ │ (Command/Tx)│   │ (NTP / Posix│     │
│  └──────┬──────┘ └──────┬──────┘   └─────────────┘     │
│         │               │                              │
│  ┌──────▼──────┐ ┌──────▼─────────────────────────┐    │
│  │ DisplayMgr  │ │ Applications:                  │    │
│  │ (LovyanGFX  │ │ - WeatherManager (Cache/Live)  │    │
│  │  Parallel8) │ │ - TaskManager (NVS Storage)    │    │
│  └──────┬──────┘ │ - TimerManager (Millis math)   │    │
│         │        │ - AlarmManager (Repeat masks)  │    │
│         │        └────────────────────────────────┘    │
│  ┌──────▼─────────────────────────────────────────┐    │
│  │ StorageManager (NVS Preferences & Flash)       │    │
│  └────────────────────────────────────────────────┘    │
│  ┌────────────────────────────────────────────────┐    │
│  │ Hardware Layer (ILI9488 8-Bit Bus, ESP32-S3)   │    │
│  └────────────────────────────────────────────────┘    │
└────────────────────────────────────────────────────────┘
```

---

## Subsystem Details

### 1. Hardware Abstraction (`include/HardwareConfig.h`)
- Defines physical pins for the 8-bit parallel bus and control strobes (`WR`, `RD`, `RS`, `CS`, `RST`).
- Strictly derived from `connection.md`.
- Requires `-D BOARD_USES_HW_GPIO_NUMBERS` build flag so constant numbers map to hardware ESP32-S3 GPIOs.

### 2. Display Engine (`DisplayManager` & `UIManager`)
- **LovyanGFX**: Native parallel 8-bit bus driver with panel ILI9488 configuration.
- **Color Scheme**: Deep obsidian dark theme (`0x0842`) with card containers, cyan accents (`0x067F`), and amber highlights (`0xFD00`).
- **Page System**:
  - `PAGE_CLOCK`: Giant typography, seconds, day of week, full date, location, weather glance.
  - `PAGE_WEATHER`: Live temperature, feels-like, condition string, humidity, barometer, wind speed, cached age indicator.
  - `PAGE_FORECAST`: 3-day projection cards with conditions and min/max temps.
  - `PAGE_TASKS`: To-do list with completion checkboxes and priority badges (`HIGH`, `MED`, `LOW`).
  - `PAGE_TIMER`: Giant MM:SS countdown timer with status indicators.
  - `PAGE_ALARM`: Configured wake-up and reminder alarms with enable state.
  - `PAGE_DEVICE_STATUS`: IP, Wi-Fi RSSI, broker connection state, free RAM heap, uptime.
- **Auto-Rotation**: Configurable carousel rotation interval (`g_config.pageIntervalMs`).

### 3. Connectivity & Remote Management (`WiFiManager` & `MQTTManager`)
- **WiFiManager**: Non-blocking connection lifecycle with periodic reconnect backoffs.
- **MQTTManager**: Automatic reconnection, Last Will and Testament (LWT) on `nexora/{device_id}/availability`, JSON status telemetry on `nexora/{device_id}/status`, and two-way command acknowledgment on `nexora/{device_id}/response`.

### 4. Persistence & Fault Resilience (`StorageManager` & `DeviceManager`)
- **StorageManager**: NVS Preferences namespaces for atomic configuration persistence, tasks, alarms, and offline weather cache.
- **DeviceManager**: Central state machine (`BOOTING`, `INITIALIZING`, `WIFI_CONNECTING`, `ONLINE`, `OFFLINE`, `ERROR`, `OTA_UPDATE`), watchdog health tracking, and memory monitoring.
- **Offline First**: All display pages (clock, cached weather, tasks, timers, alarms) operate completely independently without active network connectivity.
