#pragma once

#include <Arduino.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

class MQTTManager {
public:
    static MQTTManager& instance();

    void begin();
    void update();

    bool isConnected();
    void publishStatus();
    void publishEvent(const char* eventType, const char* detailJson = "{}");
    void sendResponse(const char* requestId, const char* command, bool success, const char* message);

private:
    MQTTManager();
    ~MQTTManager() = default;
    MQTTManager(const MQTTManager&) = delete;
    MQTTManager& operator=(const MQTTManager&) = delete;

    void connect();
    void subscribeTopics();
    void handleMessage(char* topic, byte* payload, unsigned int length);
    void handleCommand(const char* payloadStr);

    WiFiClient _wifiClient;
    PubSubClient _mqtt;

    uint32_t _lastConnectAttemptMs = 0;
    uint32_t _lastStatusPublishMs = 0;
    static constexpr uint32_t RECONNECT_INTERVAL_MS = 5000;

    char _statusTopic[64];
    char _availabilityTopic[64];
    char _commandTopic[64];
    char _responseTopic[64];
    char _weatherTopic[64];
    char _configTopic[64];
    char _eventTopic[64];
};
