#include "WeatherManager.h"
#include "StorageManager.h"
#include "TimeManager.h"

WeatherManager& WeatherManager::instance() {
    static WeatherManager inst;
    return inst;
}

void WeatherManager::begin() {
    Serial.println(F("[WEATHER] Initializing WeatherManager..."));
    memset(&_data, 0, sizeof(_data));
    strncpy(_data.location, "Kathmandu", sizeof(_data.location));
    _data.temperature = 22.0f;
    _data.feelsLike = 22.5f;
    _data.humidity = 65;
    _data.pressure = 1013;
    _data.windSpeed = 2.5f;
    strncpy(_data.condition, "Partly Cloudy", sizeof(_data.condition));
    strncpy(_data.icon, "cloudy", sizeof(_data.icon));
    _data.lastUpdated = 0;
    _data.hasData = false;
    _data.isCached = true;
    _data.forecastCount = 0;

    // Try to load cached weather from storage
    String cachedJson = StorageManager::instance().loadWeatherCache();
    if (cachedJson.length() > 10) {
        Serial.println(F("[WEATHER] Loading cached weather from storage..."));
        parseWeatherJson(cachedJson.c_str(), true);
    }
}

void WeatherManager::update() {
    // Periodic checks if needed
}

bool WeatherManager::parseWeatherJson(const char* jsonPayload, bool fromCache) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, jsonPayload);
    if (err) {
        Serial.printf("[WEATHER] JSON deserialization failed: %s\n", err.c_str());
        return false;
    }

    if (doc["location"].is<const char*>()) {
        strncpy(_data.location, doc["location"], sizeof(_data.location));
    }
    if (doc["temperature"].is<float>()) {
        _data.temperature = doc["temperature"];
    }
    if (doc["feels_like"].is<float>()) {
        _data.feelsLike = doc["feels_like"];
    }
    if (doc["humidity"].is<int>()) {
        _data.humidity = doc["humidity"];
    }
    if (doc["pressure"].is<int>()) {
        _data.pressure = doc["pressure"];
    }
    if (doc["wind_speed"].is<float>()) {
        _data.windSpeed = doc["wind_speed"];
    }
    if (doc["condition"].is<const char*>()) {
        strncpy(_data.condition, doc["condition"], sizeof(_data.condition));
    }
    if (doc["icon"].is<const char*>()) {
        strncpy(_data.icon, doc["icon"], sizeof(_data.icon));
    }
    if (doc["updated"].is<long>()) {
        _data.lastUpdated = doc["updated"];
    } else {
        _data.lastUpdated = TimeManager::instance().getEpoch();
    }

    // Forecast items
    JsonArray fArray = doc["forecast"].as<JsonArray>();
    if (!fArray.isNull()) {
        _data.forecastCount = 0;
        for (JsonObject fObj : fArray) {
            if (_data.forecastCount >= 3) break;
            ForecastItem& item = _data.forecast[_data.forecastCount];
            strncpy(item.day, fObj["day"] | "Day", sizeof(item.day));
            item.tempMax = fObj["temp_max"] | 25.0f;
            item.tempMin = fObj["temp_min"] | 15.0f;
            strncpy(item.condition, fObj["condition"] | "Clear", sizeof(item.condition));
            _data.forecastCount++;
        }
    }

    _data.hasData = true;
    _data.isCached = fromCache;

    if (!fromCache) {
        Serial.println(F("[WEATHER] Live weather data received and updated. Saving cache..."));
        StorageManager::instance().saveWeatherCache(jsonPayload);
    } else {
        Serial.println(F("[WEATHER] Cached weather loaded successfully."));
    }

    return true;
}

uint32_t WeatherManager::getAgeMinutes() const {
    if (_data.lastUpdated == 0) return 999;
    time_t current = TimeManager::instance().getEpoch();
    if (current < _data.lastUpdated) return 0;
    return (uint32_t)((current - _data.lastUpdated) / 60);
}

String WeatherManager::getAgeString() const {
    if (!_data.hasData) return "No data";
    uint32_t ageMin = getAgeMinutes();
    if (ageMin == 0) return "Updated just now";
    if (ageMin == 1) return "Updated 1 min ago";
    if (ageMin < 60) return "Updated " + String(ageMin) + " min ago";
    uint32_t hours = ageMin / 60;
    if (hours == 1) return "Updated 1 hour ago";
    return "Updated " + String(hours) + " hours ago";
}
