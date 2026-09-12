#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

struct ForecastItem {
    char day[8];
    float tempMax;
    float tempMin;
    char condition[24];
};

struct WeatherData {
    char location[32];
    float temperature;
    float feelsLike;
    int humidity;
    int pressure;
    float windSpeed;
    char condition[32];
    char icon[16];
    time_t lastUpdated;
    bool hasData;
    bool isCached;

    ForecastItem forecast[3];
    uint8_t forecastCount;
};

class WeatherManager {
public:
    static WeatherManager& instance();

    void begin();
    void update();

    bool parseWeatherJson(const char* jsonPayload, bool fromCache = false);
    const WeatherData& getData() const { return _data; }

    uint32_t getAgeMinutes() const;
    String getAgeString() const;

private:
    WeatherManager() = default;
    ~WeatherManager() = default;
    WeatherManager(const WeatherManager&) = delete;
    WeatherManager& operator=(const WeatherManager&) = delete;

    WeatherData _data;
};
