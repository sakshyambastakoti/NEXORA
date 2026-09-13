#include <Arduino.h>
#include "Config.h"
#include "HardwareConfig.h"
#include "StorageManager.h"
#include "DeviceManager.h"
#include "DisplayManager.h"
#include "UIManager.h"
#include "WiFiManager.h"
#include "TimeManager.h"
#include "WeatherManager.h"
#include "TaskManager.h"
#include "TimerManager.h"
#include "AlarmManager.h"
#include "MQTTManager.h"
#include "OTAManager.h"

// Global System Configuration
NEXORAConfig g_config;

void setup() {
    // 1. Serial Logging
    Serial.begin(115200);
    delay(200);
    Serial.println();
    Serial.println(F("=================================================="));
    Serial.println(F("  NEXORA — Smart Personal Information Station     "));
    Serial.println(F("  Production Firmware v1.0.0 (Arduino Nano ESP32) "));
    Serial.println(F("=================================================="));

    // 2. Storage System Initialization
    StorageManager& storage = StorageManager::instance();
    storage.begin();
    storage.loadConfig(g_config);

    // 3. Display Subsystem (ILI9488 8-Bit Parallel)
    DisplayManager& display = DisplayManager::instance();
    display.begin();
    display.showBootScreen("INITIALIZING SYSTEM...", 15);

    // 4. Device State Manager
    DeviceManager& device = DeviceManager::instance();
    device.begin();
    display.showBootScreen("LOADING CONFIGURATION...", 35);

    // 5. Time Subsystem
    TimeManager& timeMgr = TimeManager::instance();
    timeMgr.begin();

    // 6. Application Managers (Weather, Tasks, Timers, Alarms)
    WeatherManager& weather = WeatherManager::instance();
    weather.begin();
    display.showBootScreen("INITIALIZING APPLICATIONS...", 55);

    TaskManager& tasks = TaskManager::instance();
    tasks.begin();

    TimerManager& timer = TimerManager::instance();
    timer.begin();

    AlarmManager& alarms = AlarmManager::instance();
    alarms.begin();

    // 7. Connectivity (Wi-Fi & MQTT)
    display.showBootScreen("CONNECTING NETWORK...", 75);
    WiFiManager& wifi = WiFiManager::instance();
    wifi.begin();

    MQTTManager& mqtt = MQTTManager::instance();
    mqtt.begin();

    // 8. User Interface Engine
    display.showBootScreen("STARTING UI ENGINE...", 90);
    UIManager& ui = UIManager::instance();
    ui.begin();

    display.showBootScreen("READY!", 100);
    delay(500);

    Serial.println(F("[BOOT] NEXORA boot sequence completed successfully."));
}

void loop() {
    // High-Priority Firmware Update Mode: Dedicate all CPU & memory resources to OTA
    if (DeviceManager::instance().getState() == DeviceState::OTA_UPDATE) {
        OTAManager::instance().update();
        delay(1); // Yield to ESP32 network stack and feed FreeRTOS watchdog
        return;
    }

    // Cooperative Non-Blocking Scheduler
    WiFiManager::instance().update();
    MQTTManager::instance().update();
    TimeManager::instance().update();
    WeatherManager::instance().update();
    TaskManager::instance().update();
    TimerManager::instance().update();
    AlarmManager::instance().update();
    DisplayManager::instance().update();
    UIManager::instance().update();
    OTAManager::instance().update();
    DeviceManager::instance().update();
}
