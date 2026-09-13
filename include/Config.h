#pragma once

#include <Arduino.h>
#include "secrets.h"

// ============================================================================
// DEVICE & FIRMWARE METADATA
// ============================================================================
#define NEXORA_DEVICE_NAME        "NEXORA"
#define NEXORA_FIRMWARE_VERSION   "1.0.0"
#define NEXORA_HARDWARE_TARGET    "Arduino Nano ESP32 (ESP32-S3)"
#define NEXORA_DISPLAY_TYPE       "ILI9488 3.5\" 8-Bit Parallel"

// ============================================================================
// DEVICE SYSTEM STATES
// ============================================================================
enum class DeviceState : uint8_t {
    BOOTING,
    INITIALIZING,
    WIFI_CONNECTING,
    ONLINE,
    OFFLINE,
    ERROR,
    OTA_UPDATE
};

// ============================================================================
// UI PAGES
// ============================================================================
enum class UIPage : uint8_t {
    PAGE_CLOCK = 0,
    PAGE_WEATHER,
    PAGE_FORECAST,
    PAGE_TASKS,
    PAGE_TIMER,
    PAGE_ALARM,
    PAGE_DEVICE_STATUS,
    PAGE_COUNT
};

// ============================================================================
// RUNTIME SYSTEM CONFIGURATION STRUCTURE
// ============================================================================
struct NEXORAConfig {
    char deviceId[32];          // Unique device ID: "NEXORA-XXXX"
    char deviceName[32];        // Display Name: "NEXORA Desk"
    char timezone[48];          // Standard Timezone Name: "Asia/Kathmandu"
    char posixTz[48];           // POSIX Timezone string: "<+0545>-5:45"
    char locationName[32];      // Location: "Kathmandu"
    float latitude;             // Latitude: 27.7172
    float longitude;            // Longitude: 85.3240
    bool use24Hour;             // True: 24h format, False: 12h AM/PM
    bool useCelsius;            // True: °C, False: °F
    bool autoRotate;            // True: cycle pages automatically
    uint32_t pageIntervalMs;    // Page rotation interval in ms (e.g. 8000)
    uint32_t weatherIntervalMs; // Weather refresh request interval in ms (e.g. 600000)
    uint32_t statusIntervalMs;  // Telemetry publish interval in ms (e.g. 30000)

    // Networking
    char wifiSsid[33];
    char wifiPassword[65];
    char mqttBroker[65];
    uint16_t mqttPort;
    char mqttUser[33];
    char mqttPassword[65];

    void setDefaults() {
        snprintf(deviceId, sizeof(deviceId), "NEXORA-%04X", (uint16_t)(ESP.getEfuseMac() & 0xFFFF));
        strncpy(deviceName, "NEXORA Station", sizeof(deviceName));
        strncpy(timezone, "Asia/Kathmandu", sizeof(timezone));
        strncpy(posixTz, "<+0545>-5:45", sizeof(posixTz));
        strncpy(locationName, "Kathmandu", sizeof(locationName));
        latitude = 27.7172f;
        longitude = 85.3240f;
        use24Hour = true;
        useCelsius = true;
        autoRotate = false;
        pageIntervalMs = 15000;
        weatherIntervalMs = 600000;
        statusIntervalMs = 30000;

        strncpy(wifiSsid, DEFAULT_WIFI_SSID, sizeof(wifiSsid));
        strncpy(wifiPassword, DEFAULT_WIFI_PASSWORD, sizeof(wifiPassword));
        strncpy(mqttBroker, DEFAULT_MQTT_BROKER, sizeof(mqttBroker));
        mqttPort = DEFAULT_MQTT_PORT;
        strncpy(mqttUser, DEFAULT_MQTT_USER, sizeof(mqttUser));
        strncpy(mqttPassword, DEFAULT_MQTT_PASSWORD, sizeof(mqttPassword));
    }
};

// ============================================================================
// NVS STORAGE NAMESPACES
// ============================================================================
namespace StorageKeys {
    constexpr const char* NVS_CONFIG_NS   = "nexora_cfg";
    constexpr const char* NVS_TASKS_NS    = "nexora_tsk";
    constexpr const char* NVS_ALARMS_NS   = "nexora_alm";
    constexpr const char* NVS_WEATHER_NS  = "nexora_wtr";
}
