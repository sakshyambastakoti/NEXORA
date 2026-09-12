# NEXORA — Smart Personal Information Station
## Professional PlatformIO Firmware Development Specification

You are an experienced embedded-systems and ESP32 firmware engineer.

Your task is to design and implement production-quality firmware for **NEXORA**, a connected desktop smart information station based on an **Arduino Nano ESP32** and a **3.5-inch TFT display**.

The firmware must be modular, maintainable, reliable, offline-capable, remotely configurable, and structured as a real product rather than a simple Arduino demonstration.

---

# 1. PROJECT IDENTITY

**Device Name:** NEXORA

**Product Type:** Smart Personal Information Station

NEXORA combines:

- Digital clock
- Date and calendar information
- Online weather
- Weather forecast
- Personal tasks / to-do list
- Countdown timer
- Alarms
- Device/network status
- Remote configuration
- MQTT communication
- OTA firmware updates
- Web-based remote control
- Persistent local configuration
- Offline operation

The TFT is **display-only**.

There is **NO touchscreen** and no requirement for physical user interaction through the display.

The primary configuration interface is a remote web dashboard.

---

# 2. HARDWARE

Primary MCU:

- Arduino Nano ESP32

Display:

- 3.5-inch TFT display
- Display is output-only
- Communication/interface must be determined from the actual hardware documentation/project files

Connectivity:

- Wi-Fi
- MQTT
- NTP

Storage:

- ESP32 NVS and/or LittleFS as appropriate

Firmware development environment:

- PlatformIO
- Arduino framework for ESP32 unless there is a strong technical reason to use another framework

---

# 3. CRITICAL HARDWARE RULE

## connection.md IS THE SINGLE SOURCE OF TRUTH

Before writing ANY hardware-related code:

1. Locate `connection.md` in the project.
2. Read it completely.
3. Extract the exact pin assignments from `connection.md`.
4. Use ONLY those pin assignments.
5. Do NOT assume standard ESP32 GPIO mappings.
6. Do NOT invent GPIO numbers.
7. Do NOT replace documented pins with "better" pins without explicit instruction.
8. Do NOT copy pin assignments from generic TFT tutorials.
9. Do NOT assume the display controller or interface without checking the documentation.
10. If information required by the firmware is missing from `connection.md`, stop that portion of implementation and clearly identify the missing information.

Every GPIO used by the firmware must be traceable back to `connection.md`.

Create a central hardware configuration layer such as:

```cpp
include/HardwareConfig.h
```

or an equivalent architecture.

Do not scatter raw GPIO numbers throughout the source code.

Example structure:

```cpp
namespace Hardware {

    constexpr int TFT_CS   = ...;
    constexpr int TFT_DC   = ...;
    constexpr int TFT_RST  = ...;
    constexpr int TFT_SCLK = ...;
    constexpr int TFT_MOSI = ...;
    constexpr int TFT_MISO = ...;

}
```

The actual values MUST come from `connection.md`.

If `connection.md` defines different pins, use those pins.

---

# 4. FIRST DEVELOPMENT STEP

Before implementing the full firmware, inspect the entire project structure.

Look for:

```text
connection.md
README.md
platformio.ini
src/
include/
lib/
data/
docs/
```

Read all relevant existing documentation.

Then produce a short internal implementation plan based on the actual project files.

Do not immediately start writing a large monolithic `main.cpp`.

---

# 5. REQUIRED ARCHITECTURE

Use a modular architecture.

Recommended structure:

```text
NEXORA/
│
├── platformio.ini
│
├── README.md
│
├── connection.md
│
├── include/
│   ├── Config.h
│   ├── HardwareConfig.h
│   ├── DisplayManager.h
│   ├── WiFiManager.h
│   ├── MQTTManager.h
│   ├── WeatherManager.h
│   ├── TimeManager.h
│   ├── TaskManager.h
│   ├── TimerManager.h
│   ├── AlarmManager.h
│   ├── StorageManager.h
│   ├── OTAManager.h
│   ├── DeviceManager.h
│   └── UIManager.h
│
├── src/
│   ├── main.cpp
│   ├── DisplayManager.cpp
│   ├── WiFiManager.cpp
│   ├── MQTTManager.cpp
│   ├── WeatherManager.cpp
│   ├── TimeManager.cpp
│   ├── TaskManager.cpp
│   ├── TimerManager.cpp
│   ├── AlarmManager.cpp
│   ├── StorageManager.cpp
│   ├── OTAManager.cpp
│   ├── DeviceManager.cpp
│   └── UIManager.cpp
│
└── docs/
    ├── architecture.md
    ├── mqtt.md
    ├── display.md
    └── firmware.md
```

Adapt the structure if an existing repository already has an established architecture.

Do not unnecessarily rewrite existing working code.

---

# 6. PLATFORMIO CONFIGURATION

Configure the project professionally using `platformio.ini`.

Use the appropriate Arduino Nano ESP32 board definition supported by PlatformIO.

Include only necessary libraries.

Avoid adding large libraries without justification.

The project should support:

- Debug builds
- Production builds
- Serial logging
- OTA
- Persistent storage
- Wi-Fi
- MQTT
- JSON configuration/messages
- TFT rendering

Use appropriate compiler warnings where practical.

---

# 7. CORE FIRMWARE RESPONSIBILITIES

The firmware should contain these major subsystems:

## A. Boot System

At startup:

1. Initialize serial logging.
2. Initialize storage.
3. Load saved configuration.
4. Initialize hardware.
5. Initialize display.
6. Show NEXORA boot screen.
7. Initialize Wi-Fi.
8. Synchronize time.
9. Initialize MQTT.
10. Start weather synchronization.
11. Start OTA service.
12. Enter the main application loop.

Boot must be non-blocking wherever practical.

---

# 8. DEVICE STATES

Implement a clear device state model.

Example:

```cpp
enum class DeviceState {
    BOOTING,
    INITIALIZING,
    WIFI_CONNECTING,
    ONLINE,
    OFFLINE,
    ERROR,
    OTA_UPDATE
};
```

The actual implementation can differ, but device state must be centrally managed.

The display should be able to communicate important states such as:

```text
NEXORA

Connecting Wi-Fi...
```

or:

```text
NEXORA

OFFLINE MODE
Using cached data
```

---

# 9. WIFI MANAGEMENT

Implement robust Wi-Fi handling.

Requirements:

- Connect using stored credentials.
- Detect disconnection.
- Automatically reconnect.
- Use retry intervals.
- Do not block the entire firmware while reconnecting.
- Expose Wi-Fi status.
- Expose RSSI.
- Report IP address.
- Report connection state through MQTT.

Do not use long blocking delays for connection management.

Use `millis()` based scheduling where appropriate.

---

# 10. MQTT ARCHITECTURE

NEXORA will use MQTT as the primary remote communication protocol.

Recommended broker:

- HiveMQ Cloud or another managed MQTT broker.

Do not implement an MQTT broker on the ESP32.

The ESP32 is an MQTT client.

---

# 11. MQTT TOPIC STRUCTURE

Use the following conceptual structure:

```text
nexora/
└── {device_id}/
    ├── status
    ├── availability
    ├── config
    ├── command
    ├── display
    ├── weather
    ├── clock
    ├── tasks
    ├── timer
    ├── alarm
    └── ota
```

Example:

```text
nexora/NEXORA-WS001/status
nexora/NEXORA-WS001/config
nexora/NEXORA-WS001/command
```

The device ID must be configurable.

Prefer generating a unique default device ID from the ESP32 hardware identity/MAC address.

---

# 12. MQTT MESSAGE FORMAT

Use JSON messages.

Example status:

```json
{
    "device_id": "NEXORA-WS001",
    "online": true,
    "wifi_rssi": -48,
    "ip": "192.168.1.100",
    "firmware": "1.0.0",
    "uptime": 45231
}
```

Example configuration:

```json
{
    "location": {
        "name": "Kathmandu",
        "latitude": 27.7172,
        "longitude": 85.3240
    },
    "timezone": "Asia/Kathmandu",
    "temperature_unit": "C",
    "clock_format": "24h"
}
```

Use ArduinoJson or an equivalent lightweight JSON library.

Validate incoming JSON before applying it.

Never blindly trust MQTT payloads.

---

# 13. MQTT COMMAND SYSTEM

Implement a command dispatcher.

Commands may include:

```text
SET_CONFIG
REFRESH_WEATHER
SET_DISPLAY
SET_TASK
DELETE_TASK
SET_TIMER
START_TIMER
PAUSE_TIMER
STOP_TIMER
SET_ALARM
DELETE_ALARM
SYNC_TIME
RESTART
REQUEST_STATUS
START_OTA
```

Example:

```json
{
    "command": "SET_CONFIG",
    "data": {
        "clock_format": "24h",
        "temperature_unit": "C"
    }
}
```

The command system must be extensible.

Avoid creating hundreds of unrelated MQTT callbacks.

Use a centralized dispatcher.

---

# 14. MQTT RELIABILITY

Implement:

- Automatic reconnect
- Keep-alive
- Last Will / availability where supported
- Online/offline state
- Message validation
- QoS appropriate to message type
- Retained configuration where appropriate
- Duplicate command protection where useful

Critical configuration should not be lost if the device temporarily disconnects.

---

# 15. TIME MANAGEMENT

Use NTP for accurate time synchronization.

Requirements:

- Configurable timezone
- Automatic NTP synchronization
- Local clock operation after synchronization
- 12/24-hour display option
- Date
- Day of week
- Unix timestamp internally where useful

Default timezone:

```text
Asia/Kathmandu
```

But do not hard-code it as the only possible timezone.

Make timezone configurable.

Do not repeatedly call NTP every second.

Synchronize periodically.

---

# 16. WEATHER SYSTEM

There are NO physical weather sensors.

Weather information must come from an online weather API.

The device should NOT require a physical temperature, humidity, pressure, or rain sensor.

Weather configuration must support:

```text
Location name
Latitude
Longitude
Temperature unit
Weather provider
Refresh interval
```

Prefer obtaining weather through the backend/web infrastructure rather than exposing API keys directly inside the firmware.

Ideal architecture:

```text
Weather API
     │
     ▼
Backend / Web service
     │
     ▼
MQTT
     │
     ▼
NEXORA
     │
     ▼
TFT
```

The ESP32 should receive normalized weather JSON.

Example:

```json
{
    "temperature": 24.5,
    "feels_like": 25.1,
    "humidity": 68,
    "pressure": 1012,
    "wind_speed": 3.2,
    "condition": "Cloudy",
    "icon": "cloudy",
    "updated": 1757654400
}
```

Weather must be cached locally.

If the internet disappears:

```text
Weather
24°C
Cloudy

Last updated
42 min ago
```

Do not display fake live weather data.

Clearly indicate stale/cached data.

---

# 17. DISPLAY SYSTEM

The TFT is output-only.

There must be no touch UI assumptions.

Create a dedicated display abstraction.

Example:

```cpp
DisplayManager
```

Responsibilities:

- Initialize TFT
- Draw UI
- Manage screen transitions
- Render icons
- Render text
- Render progress indicators
- Render status
- Manage screen refresh

Do not put all drawing code inside `loop()`.

---

# 18. DISPLAY PAGES

Implement a page-based UI system.

Suggested pages:

```text
PAGE_CLOCK
PAGE_WEATHER
PAGE_FORECAST
PAGE_TASKS
PAGE_TIMER
PAGE_ALARM
PAGE_DEVICE_STATUS
```

The exact number may be expanded later.

Because there is no touchscreen, pages should automatically rotate.

Example:

```text
Clock → Weather → Forecast → Tasks → Timer → Status
```

Make the page rotation interval configurable.

Example:

```json
{
    "auto_rotate": true,
    "page_interval": 8
}
```

---

# 19. CLOCK SCREEN

Create a clean premium clock UI.

Display:

```text
12:48:32

Friday
12 September 2026

Kathmandu
```

Optional:

- seconds
- date
- day
- timezone
- Wi-Fi indicator
- MQTT indicator
- weather summary

Do not overcrowd the display.

The clock should remain readable from a distance.

---

# 20. WEATHER SCREEN

Display:

```text
KATHMANDU

24°C
Cloudy

Feels like 25°C
Humidity 68%
Wind 3.2 m/s

Updated 10 min ago
```

Use graphical weather icons if supported by the display library.

Icons should not block the main UI thread.

---

# 21. TASK SYSTEM

Implement a local task/to-do manager.

Each task should support fields such as:

```json
{
    "id": "task_001",
    "title": "Complete NEXORA firmware",
    "completed": false,
    "priority": 2,
    "created_at": 1757654400
}
```

Requirements:

- Add task remotely
- Delete task remotely
- Mark complete
- Mark incomplete
- Persist tasks locally
- Display tasks on TFT
- Synchronize through MQTT

Limit the maximum number of locally stored tasks to a sensible configurable value.

---

# 22. TIMER SYSTEM

Implement a countdown timer.

Features:

- Set duration remotely
- Start
- Pause
- Resume
- Stop
- Reset
- Remaining time
- Completion event

Example:

```text
TIMER

24:38

RUNNING
```

On completion:

```text
TIMER COMPLETE
```

Publish an MQTT event:

```json
{
    "event": "TIMER_COMPLETE",
    "timer_id": "timer_001"
}
```

Timer logic must use `millis()` or timestamps rather than blocking delays.

---

# 23. ALARM SYSTEM

Implement configurable alarms.

Support:

- Hour
- Minute
- Enabled/disabled
- Repeat days
- Alarm name

Example:

```json
{
    "id": "alarm_001",
    "hour": 6,
    "minute": 30,
    "enabled": true,
    "repeat": [
        "MON",
        "TUE",
        "WED",
        "THU",
        "FRI"
    ]
}
```

When an alarm triggers, publish an MQTT event.

Since hardware audio/buzzer has not been specified, do not invent an audio output.

Instead implement an alarm event/state that can later be connected to hardware.

---

# 24. LOCAL STORAGE

Use ESP32 persistent storage.

Separate persistent data logically:

```text
/config
/tasks
/timers
/alarms
/weather_cache
/device
```

Use NVS for small configuration values.

Use LittleFS for larger structured data if appropriate.

Requirements:

- Load on boot
- Save when changed
- Avoid unnecessary flash writes
- Validate stored data
- Recover gracefully from corrupted data
- Provide factory-reset capability

Do not write to flash every loop iteration.

---

# 25. OFFLINE-FIRST DESIGN

NEXORA must remain useful without internet.

Offline functionality:

- Clock
- Date
- Cached weather
- Tasks
- Timer
- Alarms
- Display
- Local configuration

Online functionality:

- Weather updates
- MQTT
- Remote configuration
- Remote tasks
- Remote timers
- Remote alarms
- OTA
- Device monitoring

The firmware must never continuously reboot simply because Wi-Fi or MQTT is unavailable.

---

# 26. OTA UPDATES

Implement secure OTA architecture.

The firmware should support remote firmware updates.

Potential flow:

```text
Web Dashboard
      │
      ▼
OTA Request
      │
      ▼
MQTT
      │
      ▼
NEXORA
      │
      ▼
Firmware Download
      │
      ▼
Validation
      │
      ▼
Update
      │
      ▼
Reboot
```

Do not implement unsafe firmware flashing logic.

Validate:

- Firmware version
- Download status
- Image integrity
- Available partition space

Do not brick the device if OTA fails.

---

# 27. DEVICE STATUS

Publish periodic telemetry.

Example:

```json
{
    "device_id": "NEXORA-WS001",
    "uptime": 98231,
    "free_heap": 184320,
    "wifi_rssi": -51,
    "mqtt_connected": true,
    "time_synced": true,
    "weather_age": 420,
    "firmware": "1.0.0"
}
```

Publish at a reasonable interval.

Do not publish excessively.

---

# 28. WATCHDOG / FAULT HANDLING

Implement robust fault handling.

Avoid:

```cpp
delay(10000);
```

for normal application behavior.

Prefer cooperative scheduling:

```cpp
millis()
```

or task-based scheduling where appropriate.

Handle:

- Wi-Fi failure
- MQTT failure
- NTP failure
- Weather failure
- Invalid JSON
- Display initialization failure
- Storage failure
- OTA failure

The device should fail gracefully.

---

# 29. MAIN LOOP ARCHITECTURE

The main loop should remain lightweight.

Conceptually:

```cpp
void loop() {

    wifiManager.update();

    mqttManager.update();

    timeManager.update();

    weatherManager.update();

    taskManager.update();

    timerManager.update();

    alarmManager.update();

    displayManager.update();

    otaManager.update();

    deviceManager.update();

}
```

Do not put huge blocks of application logic inside `loop()`.

---

# 30. NON-BLOCKING REQUIREMENT

NEXORA must be designed as a responsive embedded product.

Avoid long blocking operations.

Do not use:

```cpp
delay(5000);
```

for network retries, display page switching, weather updates, or timers.

Instead use timestamp-based scheduling.

Example:

```cpp
if (millis() - lastUpdate >= updateInterval) {
    lastUpdate = millis();
    performUpdate();
}
```

---

# 31. CONFIGURATION MANAGEMENT

Create a central configuration model.

Example:

```cpp
struct NEXORAConfig {
    String deviceName;
    String timezone;
    String locationName;

    float latitude;
    float longitude;

    bool use24Hour;
    bool useCelsius;

    bool autoRotate;
    uint32_t pageInterval;

    uint32_t weatherInterval;
};
```

Adapt fields based on the actual requirements.

Configuration should be:

- Loaded from storage
- Updated through MQTT
- Validated
- Saved
- Applied without unnecessary reboot

---

# 32. SECURITY

Do not hard-code production credentials into source code.

Never commit:

- Wi-Fi passwords
- MQTT passwords
- API keys
- private tokens

Use a secure configuration mechanism.

During development, support a local secrets mechanism appropriate for PlatformIO.

Example:

```text
include/secrets.h
```

and make sure it is excluded from Git.

For production, use secure credential provisioning.

---

# 33. LOGGING

Implement structured serial logs.

Example:

```text
[BOOT] NEXORA firmware v1.0.0
[INIT] Storage initialized
[INIT] Display initialized
[WIFI] Connecting...
[WIFI] Connected
[WIFI] RSSI: -48 dBm
[NTP] Time synchronized
[MQTT] Connected
[WEATHER] Cache loaded
[READY] NEXORA online
```

Use log levels where practical:

```text
ERROR
WARN
INFO
DEBUG
```

Debug logging should be easy to disable for production builds.

---

# 34. ERROR HANDLING

Never silently ignore critical failures.

Bad:

```cpp
mqtt.connect();
```

Better:

```text
[MQTT] Connection failed
[MQTT] Retry in 5 seconds
```

But do not flood Serial output.

Use controlled retry intervals.

---

# 35. CODE QUALITY REQUIREMENTS

Follow professional embedded coding practices.

Requirements:

- Meaningful names
- Small functions
- Clear interfaces
- Header/source separation
- No giant monolithic file
- No duplicated logic
- No unexplained magic numbers
- Constants in configuration
- Comments explaining WHY, not obvious WHAT
- RAII/resource safety where applicable
- Avoid unnecessary dynamic memory allocation
- Avoid `String` abuse where memory fragmentation could become a concern
- Validate external data
- Check return values
- Avoid blocking calls

---

# 36. MEMORY MANAGEMENT

ESP32 has limited embedded resources.

Monitor:

- Heap
- Stack
- Flash
- JSON document sizes
- Display frame buffers

Do not allocate large buffers repeatedly.

Avoid memory leaks.

Be especially careful with:

- MQTT payloads
- JSON parsing
- TFT image buffers
- HTTP responses
- OTA downloads

---

# 37. WEB DASHBOARD COMPATIBILITY

The firmware should be designed for a Vercel-hosted web dashboard.

The architecture is:

```text
                 NEXORA WEB DASHBOARD
                       │
                    Vercel
                       │
                MQTT/WebSocket
                       │
                Managed MQTT
                  Broker
                       │
                     Wi-Fi
                       │
                  NEXORA ESP32
                       │
                     TFT
```

The ESP32 should not depend on the Vercel frontend being online for basic operation.

---

# 38. DEVICE COMMAND ACKNOWLEDGEMENT

For important remote commands, support acknowledgement.

Example request:

```json
{
    "request_id": "req_7821",
    "command": "SET_CONFIG",
    "data": {
        "use24Hour": true
    }
}
```

Response:

```json
{
    "request_id": "req_7821",
    "success": true,
    "command": "SET_CONFIG"
}
```

This allows the web dashboard to know whether the device actually applied the command.

---

# 39. SYNCHRONIZATION MODEL

Use a clear synchronization strategy.

Example:

```text
WEB DASHBOARD
      │
      │ configuration
      ▼
    MQTT
      │
      ▼
   NEXORA
      │
      │ acknowledgement
      ▼
    MQTT
      │
      ▼
WEB DASHBOARD
```

Local storage remains the device's fallback state.

---

# 40. FACTORY RESET

Implement a software factory-reset mechanism.

For example:

```text
FACTORY_RESET
```

received through an authenticated command.

It should reset:

- Configuration
- Tasks
- Alarms
- Timers
- Cached weather

but should not unnecessarily destroy firmware itself.

After reset:

```text
NEXORA
Factory reset complete
Restarting...
```

---

# 41. DISPLAY DESIGN PRINCIPLES

NEXORA should look like a commercial smart information station.

Use:

- Clear hierarchy
- Large typography for time
- Minimal information density
- Consistent spacing
- Consistent icons
- Smooth page transitions where practical
- Small status indicators
- Clear offline/online indication

Do not create a cluttered Arduino-demo UI.

---

# 42. FUTURE EXTENSIBILITY

Design the firmware so future features can be added without rewriting the core.

Possible future modules:

```text
Calendar
Pomodoro
Notes
Air-quality API
News
Crypto/stock information
Custom dashboards
Smart-home control
Voice control
Bluetooth provisioning
Multiple weather locations
Multiple NEXORA devices
Remote diagnostics
Remote logs
```

Do not implement these unless explicitly requested.

Design interfaces so they can be added later.

---

# 43. DOCUMENTATION REQUIREMENTS

After implementing the firmware, update/create:

```text
README.md
connection.md
docs/architecture.md
docs/mqtt.md
docs/firmware.md
```

Document:

- Hardware
- Pin mapping
- Software architecture
- MQTT topics
- JSON payloads
- Configuration
- Build instructions
- Flashing instructions
- OTA procedure
- Troubleshooting
- Development setup

Do not modify `connection.md` pin assignments unless the user explicitly requests a hardware change.

---

# 44. IMPLEMENTATION ORDER

Implement in stages.

## Phase 1 — Hardware foundation

- Read `connection.md`
- Configure PlatformIO
- Configure exact GPIOs
- Initialize TFT
- Test basic display rendering
- Test Serial logging

## Phase 2 — Core system

- Configuration
- Storage
- Device state
- Time management

## Phase 3 — Connectivity

- Wi-Fi
- MQTT
- MQTT reconnect
- Status publishing

## Phase 4 — Application

- Clock
- Weather
- Tasks
- Timer
- Alarms

## Phase 5 — UI

- Page system
- Auto rotation
- Status indicators
- Error screens
- Offline screens

## Phase 6 — Remote management

- Remote configuration
- MQTT commands
- Command acknowledgement
- OTA

## Phase 7 — Reliability

- Error handling
- Watchdog
- Memory monitoring
- Persistent-state validation
- Recovery testing

## Phase 8 — Documentation

- Architecture
- MQTT documentation
- Build instructions
- Troubleshooting

---

# 45. TESTING REQUIREMENTS

Create meaningful tests/checks for:

### Hardware

- TFT initialization
- Correct orientation
- Correct resolution
- Correct color rendering
- Text rendering

### Wi-Fi

- Initial connection
- Wrong credentials
- Network unavailable
- Network recovery

### MQTT

- Broker connection
- Reconnection
- Subscribe
- Publish
- Invalid JSON
- Command acknowledgement

### Time

- NTP synchronization
- Timezone
- Offline timekeeping

### Weather

- Valid weather payload
- Invalid payload
- Missing values
- Stale weather
- Offline mode

### Storage

- First boot
- Save
- Reload
- Corrupted data
- Factory reset

### Timer

- Start
- Pause
- Resume
- Stop
- Completion

### Alarm

- Trigger
- Disable
- Repeat days

### OTA

- Valid firmware
- Failed download
- Invalid firmware
- Recovery

---

# 46. IMPORTANT DEVELOPMENT RULE

Do NOT implement everything in one pass if that would create unstable firmware.

Build incrementally.

After each major subsystem:

1. Compile.
2. Check warnings.
3. Fix errors.
4. Test.
5. Verify memory usage.
6. Continue.

Do not hide compilation errors.

Do not replace broken code with arbitrary workarounds.

---

# 47. HARDWARE ASSUMPTION RULE

If you encounter something like:

```text
TFT_CS
TFT_DC
TFT_RST
TFT_MOSI
TFT_MISO
TFT_SCLK
```

DO NOT assign GPIO numbers from memory.

Instead:

```text
Read connection.md
       ↓
Find exact pin
       ↓
Put pin into HardwareConfig
       ↓
Use HardwareConfig everywhere
```

If `connection.md` says:

```text
TFT_CS = GPIO XX
```

then use that exact GPIO.

If the display controller is not documented, inspect available project documentation or ask for the exact display controller/model rather than guessing.

---

# 48. FINAL ACCEPTANCE CRITERIA

The firmware is considered complete only when:

- PlatformIO builds successfully.
- No undefined GPIO assumptions exist.
- All hardware pins originate from `connection.md`.
- TFT works correctly.
- NEXORA boot sequence works.
- Clock works offline.
- NTP synchronization works online.
- Wi-Fi reconnects automatically.
- MQTT reconnects automatically.
- MQTT commands work.
- MQTT status is published.
- Weather data is received and cached.
- Weather remains available as cached data offline.
- Tasks persist after reboot.
- Timer works without blocking.
- Alarms work based on synchronized time.
- Display pages work without touchscreen.
- Configuration persists after reboot.
- Remote configuration works.
- Command acknowledgements work.
- OTA architecture works safely.
- Errors do not cause continuous reboot loops.
- Serial logs are useful and controlled.
- Firmware is modular.
- Documentation is updated.

---

# 49. FINAL INSTRUCTION TO THE CODING AGENT

Treat NEXORA as a **real embedded product**, not an Arduino tutorial.

Prioritize:

1. Correct hardware integration
2. `connection.md` accuracy
3. Reliability
4. Offline operation
5. Modular architecture
6. MQTT reliability
7. Persistent configuration
8. Clean TFT UI
9. Remote management
10. OTA capability
11. Security
12. Maintainability

Most importantly:

**NEVER GUESS HARDWARE PINS.**

**ALWAYS READ `connection.md` FIRST AND USE ITS EXACT PINOUT.**

Before generating hardware-dependent code, verify the actual contents of:

```text
connection.md
```

If the required hardware information is missing or contradictory, explicitly report the issue instead of inventing a solution.

Begin by inspecting the project files and `connection.md`, then create the implementation plan and proceed with the firmware incrementally.