#include "MQTTManager.h"
#include "Config.h"
#include "DeviceManager.h"
#include "StorageManager.h"
#include "TimeManager.h"
#include "WeatherManager.h"
#include "TaskManager.h"
#include "TimerManager.h"
#include "AlarmManager.h"
#include "UIManager.h"
#include "WiFiManager.h"

extern NEXORAConfig g_config;

MQTTManager& MQTTManager::instance() {
    static MQTTManager inst;
    return inst;
}

MQTTManager::MQTTManager() : _mqtt(_wifiClient) {}

void MQTTManager::begin() {
    Serial.println(F("[MQTT] Initializing MQTT Manager..."));

    // Prepare topic strings
    snprintf(_statusTopic, sizeof(_statusTopic), "nexora/%s/status", g_config.deviceId);
    snprintf(_availabilityTopic, sizeof(_availabilityTopic), "nexora/%s/availability", g_config.deviceId);
    snprintf(_commandTopic, sizeof(_commandTopic), "nexora/%s/command", g_config.deviceId);
    snprintf(_responseTopic, sizeof(_responseTopic), "nexora/%s/response", g_config.deviceId);
    snprintf(_weatherTopic, sizeof(_weatherTopic), "nexora/%s/weather", g_config.deviceId);
    snprintf(_configTopic, sizeof(_configTopic), "nexora/%s/config", g_config.deviceId);
    snprintf(_eventTopic, sizeof(_eventTopic), "nexora/%s/event", g_config.deviceId);

    _mqtt.setServer(g_config.mqttBroker, g_config.mqttPort);
    _mqtt.setBufferSize(2048);
    _mqtt.setCallback([this](char* topic, byte* payload, unsigned int length) {
        this->handleMessage(topic, payload, length);
    });
}

void MQTTManager::connect() {
    if (!WiFiManager::instance().isConnected()) return;
    if (strlen(g_config.mqttBroker) == 0) return;

    Serial.printf("[MQTT] Connecting to broker %s:%d as %s...\n", 
                  g_config.mqttBroker, g_config.mqttPort, g_config.deviceId);

    bool ok = false;
    if (strlen(g_config.mqttUser) > 0) {
        ok = _mqtt.connect(g_config.deviceId, g_config.mqttUser, g_config.mqttPassword,
                           _availabilityTopic, 1, true, "offline");
    } else {
        ok = _mqtt.connect(g_config.deviceId,
                           _availabilityTopic, 1, true, "offline");
    }

    if (ok) {
        Serial.println(F("[MQTT] Connected to broker successfully!"));
        DeviceManager::instance().setState(DeviceState::ONLINE);

        // Publish availability (retained)
        _mqtt.publish(_availabilityTopic, "online", true);

        // Subscribe to topics
        subscribeTopics();

        // Send initial status
        publishStatus();
    } else {
        Serial.printf("[MQTT] Connection failed, rc=%d. Will retry in 5s\n", _mqtt.state());
    }
}

void MQTTManager::subscribeTopics() {
    _mqtt.subscribe(_commandTopic);
    _mqtt.subscribe(_configTopic);
    _mqtt.subscribe(_weatherTopic);
    Serial.printf("[MQTT] Subscribed to %s, %s, %s\n", _commandTopic, _configTopic, _weatherTopic);
}

void MQTTManager::update() {
    if (WiFiManager::instance().isConnected()) {
        if (!_mqtt.connected()) {
            if (DeviceManager::instance().getState() == DeviceState::ONLINE) {
                DeviceManager::instance().setState(DeviceState::OFFLINE);
            }
            if (millis() - _lastConnectAttemptMs >= RECONNECT_INTERVAL_MS) {
                _lastConnectAttemptMs = millis();
                connect();
            }
        } else {
            _mqtt.loop();

            // Periodic status telemetry
            if (millis() - _lastStatusPublishMs >= g_config.statusIntervalMs) {
                _lastStatusPublishMs = millis();
                publishStatus();
            }

            // Check for Timer complete event
            if (TimerManager::instance().hasPendingCompleteEvent()) {
                publishEvent("TIMER_COMPLETE", "{\"message\":\"Timer reached zero\"}");
            }

            // Check for Alarm trigger event
            String alarmId;
            if (AlarmManager::instance().hasPendingTriggerEvent(alarmId)) {
                String detail = "{\"alarm_id\":\"" + alarmId + "\"}";
                publishEvent("ALARM_TRIGGERED", detail.c_str());
            }
        }
    }
}

bool MQTTManager::isConnected() {
    return _mqtt.connected();
}

void MQTTManager::publishStatus() {
    if (!_mqtt.connected()) return;

    int8_t rssi = WiFiManager::instance().getRSSI();
    bool timeSynced = TimeManager::instance().isSynced();
    uint32_t weatherAge = WeatherManager::instance().getAgeMinutes() * 60;

    String statusJson = DeviceManager::instance().getStatusJson(rssi, true, timeSynced, weatherAge);
    _mqtt.publish(_statusTopic, statusJson.c_str(), false);
    Serial.println(F("[MQTT] Published status telemetry."));
}

void MQTTManager::publishEvent(const char* eventType, const char* detailJson) {
    if (!_mqtt.connected()) return;

    JsonDocument doc;
    doc["device_id"] = g_config.deviceId;
    doc["event"] = eventType;
    doc["timestamp"] = (long)TimeManager::instance().getEpoch();

    JsonDocument detailDoc;
    deserializeJson(detailDoc, detailJson);
    doc["data"] = detailDoc;

    String out;
    serializeJson(doc, out);
    _mqtt.publish(_eventTopic, out.c_str(), false);
    Serial.printf("[MQTT] Published event: %s\n", eventType);
}

void MQTTManager::sendResponse(const char* requestId, const char* command, bool success, const char* message) {
    if (!_mqtt.connected()) return;

    JsonDocument doc;
    doc["request_id"] = requestId;
    doc["command"] = command;
    doc["success"] = success;
    doc["message"] = message;
    doc["timestamp"] = (long)TimeManager::instance().getEpoch();

    String out;
    serializeJson(doc, out);
    _mqtt.publish(_responseTopic, out.c_str(), false);
}

void MQTTManager::handleMessage(char* topic, byte* payload, unsigned int length) {
    char payloadStr[length + 1];
    memcpy(payloadStr, payload, length);
    payloadStr[length] = '\0';

    Serial.printf("[MQTT] Message received on topic: %s\n", topic);

    if (strcmp(topic, _weatherTopic) == 0) {
        WeatherManager::instance().parseWeatherJson(payloadStr, false);
        UIManager::instance().forceRedraw();
    } else if (strcmp(topic, _commandTopic) == 0) {
        handleCommand(payloadStr);
    } else if (strcmp(topic, _configTopic) == 0) {
        // Direct config update
        JsonDocument doc;
        if (!deserializeJson(doc, payloadStr)) {
            if (doc["location"].is<const char*>()) strncpy(g_config.locationName, doc["location"], sizeof(g_config.locationName));
            if (doc["timezone"].is<const char*>()) strncpy(g_config.timezone, doc["timezone"], sizeof(g_config.timezone));
            if (doc["posixTz"].is<const char*>()) {
                strncpy(g_config.posixTz, doc["posixTz"], sizeof(g_config.posixTz));
                TimeManager::instance().setTimezone(g_config.posixTz);
            }
            if (doc["use24Hour"].is<bool>()) g_config.use24Hour = doc["use24Hour"];
            if (doc["useCelsius"].is<bool>()) g_config.useCelsius = doc["useCelsius"];
            if (doc["autoRotate"].is<bool>()) g_config.autoRotate = doc["autoRotate"];
            if (doc["pageInterval"].is<uint32_t>()) g_config.pageIntervalMs = doc["pageInterval"];
            StorageManager::instance().saveConfig(g_config);
            UIManager::instance().forceRedraw();
        }
    }
}

void MQTTManager::handleCommand(const char* payloadStr) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payloadStr);
    if (err) {
        Serial.printf("[MQTT] Malformed command JSON: %s\n", err.c_str());
        return;
    }

    const char* reqId = doc["request_id"] | "none";
    const char* cmd = doc["command"] | "";
    JsonObject data = doc["data"].as<JsonObject>();

    Serial.printf("[MQTT] Executing command '%s' (Req: %s)\n", cmd, reqId);

    if (strcmp(cmd, "SET_CONFIG") == 0) {
        if (data["device_name"].is<const char*>()) strncpy(g_config.deviceName, data["device_name"], sizeof(g_config.deviceName));
        if (data["timezone"].is<const char*>()) strncpy(g_config.timezone, data["timezone"], sizeof(g_config.timezone));
        if (data["posix_tz"].is<const char*>()) {
            strncpy(g_config.posixTz, data["posix_tz"], sizeof(g_config.posixTz));
            TimeManager::instance().setTimezone(g_config.posixTz);
        }
        if (data["location_name"].is<const char*>()) strncpy(g_config.locationName, data["location_name"], sizeof(g_config.locationName));
        if (data["latitude"].is<float>()) g_config.latitude = data["latitude"];
        if (data["longitude"].is<float>()) g_config.longitude = data["longitude"];
        if (data["use_24hour"].is<bool>()) g_config.use24Hour = data["use_24hour"];
        if (data["use_celsius"].is<bool>()) g_config.useCelsius = data["use_celsius"];
        if (data["auto_rotate"].is<bool>()) g_config.autoRotate = data["auto_rotate"];
        if (data["page_interval_ms"].is<uint32_t>()) g_config.pageIntervalMs = data["page_interval_ms"];

        StorageManager::instance().saveConfig(g_config);
        UIManager::instance().forceRedraw();
        sendResponse(reqId, cmd, true, "Configuration updated and saved");
    } else if (strcmp(cmd, "SET_DISPLAY") == 0) {
        if (data["auto_rotate"].is<bool>()) {
            g_config.autoRotate = data["auto_rotate"];
        }
        if (data["page"].is<const char*>()) {
            const char* p = data["page"];
            if (strcasecmp(p, "clock") == 0) UIManager::instance().setPage(UIPage::PAGE_CLOCK);
            else if (strcasecmp(p, "weather") == 0) UIManager::instance().setPage(UIPage::PAGE_WEATHER);
            else if (strcasecmp(p, "forecast") == 0) UIManager::instance().setPage(UIPage::PAGE_FORECAST);
            else if (strcasecmp(p, "tasks") == 0) UIManager::instance().setPage(UIPage::PAGE_TASKS);
            else if (strcasecmp(p, "timer") == 0) UIManager::instance().setPage(UIPage::PAGE_TIMER);
            else if (strcasecmp(p, "alarm") == 0) UIManager::instance().setPage(UIPage::PAGE_ALARM);
            else if (strcasecmp(p, "status") == 0) UIManager::instance().setPage(UIPage::PAGE_DEVICE_STATUS);
        } else if (data["page_index"].is<int>()) {
            UIManager::instance().setPage(static_cast<UIPage>(data["page_index"].as<int>()));
        }
        sendResponse(reqId, cmd, true, "Display page updated");
    } else if (strcmp(cmd, "SET_TASK") == 0) {
        const char* id = data["id"] | "t_new";
        const char* title = data["title"] | "Untitled Task";
        uint8_t pri = data["priority"] | 1;
        bool ok = TaskManager::instance().addTask(id, title, pri);
        UIManager::instance().forceRedraw();
        sendResponse(reqId, cmd, ok, ok ? "Task added/updated" : "Task limit reached");
    } else if (strcmp(cmd, "DELETE_TASK") == 0) {
        const char* id = data["id"] | "";
        bool ok = TaskManager::instance().deleteTask(id);
        UIManager::instance().forceRedraw();
        sendResponse(reqId, cmd, ok, ok ? "Task deleted" : "Task not found");
    } else if (strcmp(cmd, "TOGGLE_TASK") == 0) {
        const char* id = data["id"] | "";
        bool ok = TaskManager::instance().toggleTask(id);
        UIManager::instance().forceRedraw();
        sendResponse(reqId, cmd, ok, ok ? "Task toggled" : "Task not found");
    } else if (strcmp(cmd, "SET_TIMER") == 0) {
        uint32_t dur = data["duration"] | 1500;
        TimerManager::instance().setDuration(dur);
        UIManager::instance().forceRedraw();
        sendResponse(reqId, cmd, true, "Timer duration set");
    } else if (strcmp(cmd, "START_TIMER") == 0) {
        TimerManager::instance().start();
        UIManager::instance().forceRedraw();
        sendResponse(reqId, cmd, true, "Timer started");
    } else if (strcmp(cmd, "PAUSE_TIMER") == 0) {
        TimerManager::instance().pause();
        UIManager::instance().forceRedraw();
        sendResponse(reqId, cmd, true, "Timer paused");
    } else if (strcmp(cmd, "STOP_TIMER") == 0) {
        TimerManager::instance().stop();
        UIManager::instance().forceRedraw();
        sendResponse(reqId, cmd, true, "Timer stopped");
    } else if (strcmp(cmd, "SET_ALARM") == 0) {
        const char* id = data["id"] | "alm_1";
        const char* name = data["name"] | "Alarm";
        uint8_t hour = data["hour"] | 7;
        uint8_t min = data["minute"] | 0;
        bool enabled = data["enabled"] | true;
        uint8_t mask = data["repeat_mask"] | 0b01111111;
        bool ok = AlarmManager::instance().addOrUpdateAlarm(id, name, hour, min, enabled, mask);
        UIManager::instance().forceRedraw();
        sendResponse(reqId, cmd, ok, ok ? "Alarm saved" : "Alarm limit reached");
    } else if (strcmp(cmd, "DELETE_ALARM") == 0) {
        const char* id = data["id"] | "";
        bool ok = AlarmManager::instance().deleteAlarm(id);
        UIManager::instance().forceRedraw();
        sendResponse(reqId, cmd, ok, ok ? "Alarm deleted" : "Alarm not found");
    } else if (strcmp(cmd, "TOGGLE_ALARM") == 0) {
        const char* id = data["id"] | "";
        bool ok = AlarmManager::instance().toggleAlarm(id);
        UIManager::instance().forceRedraw();
        sendResponse(reqId, cmd, ok, ok ? "Alarm toggled" : "Alarm not found");
    } else if (strcmp(cmd, "SYNC_TIME") == 0) {
        TimeManager::instance().syncNTP();
        sendResponse(reqId, cmd, true, "NTP synchronization initiated");
    } else if (strcmp(cmd, "REQUEST_STATUS") == 0) {
        publishStatus();
        sendResponse(reqId, cmd, true, "Telemetry status published");
    } else if (strcmp(cmd, "RESTART") == 0) {
        sendResponse(reqId, cmd, true, "Device restarting now");
        DeviceManager::instance().restartDevice("Remote restart request");
    } else if (strcmp(cmd, "FACTORY_RESET") == 0) {
        sendResponse(reqId, cmd, true, "Executing factory reset");
        DeviceManager::instance().triggerFactoryReset();
    } else {
        sendResponse(reqId, cmd, false, "Unknown or unsupported command");
    }
}
