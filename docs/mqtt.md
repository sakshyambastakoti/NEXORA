# NEXORA MQTT Communication Specification

NEXORA communicates remotely with dashboards and backend services using JSON-formatted MQTT messages.

Broker recommended: **HiveMQ Cloud**, Mosquitto, or any standard MQTT 3.1.1 broker.

---

## 1. Topic Hierarchy

All topics are prefixed by `nexora/{device_id}/`, where `{device_id}` is a unique identifier (e.g. `NEXORA-A1B2`):

| Topic | Direction | QoS | Retained | Purpose |
| :--- | :--- | :--- | :--- | :--- |
| `nexora/{device_id}/availability` | Device → Broker | 1 | True | Online / Offline status (LWT) |
| `nexora/{device_id}/status` | Device → Broker | 0 | False | Periodic telemetry status |
| `nexora/{device_id}/event` | Device → Broker | 1 | False | Real-time events (Timer, Alarm) |
| `nexora/{device_id}/response` | Device → Broker | 1 | False | Command execution acknowledgements |
| `nexora/{device_id}/command` | Dashboard → Device | 1 | False | Remote command dispatcher |
| `nexora/{device_id}/config` | Dashboard → Device | 1 | True | Direct configuration updates |
| `nexora/{device_id}/weather` | Backend → Device | 1 | True | Weather payload injection |

---

## 2. Device Status Telemetry (`.../status`)

Published periodically (default: every 30 seconds):
```json
{
  "device_id": "NEXORA-A1B2",
  "device_name": "NEXORA Desk",
  "firmware": "1.0.0",
  "state": "ONLINE",
  "uptime": 45231,
  "free_heap": 184320,
  "min_free_heap": 178940,
  "wifi_rssi": -52,
  "mqtt_connected": true,
  "time_synced": true,
  "weather_age": 120
}
```

---

## 3. Remote Commands (`.../command`) & Responses (`.../response`)

All commands support a `request_id` for end-to-end confirmation.

### Command Format
```json
{
  "request_id": "req_101",
  "command": "COMMAND_NAME",
  "data": { ... }
}
```

### Response / ACK Format
```json
{
  "request_id": "req_101",
  "command": "COMMAND_NAME",
  "success": true,
  "message": "Configuration updated and saved",
  "timestamp": 1757654400
}
```

### Supported Commands

#### A. `SET_CONFIG`
```json
{
  "request_id": "c1",
  "command": "SET_CONFIG",
  "data": {
    "device_name": "Office NEXORA",
    "timezone": "Asia/Kathmandu",
    "posix_tz": "<+0545>-5:45",
    "location_name": "Kathmandu",
    "use_24hour": true,
    "use_celsius": true,
    "auto_rotate": true,
    "page_interval_ms": 10000
  }
}
```

#### B. `SET_DISPLAY`
Switch the active display page immediately or toggle auto-rotation:
```json
{
  "request_id": "c2",
  "command": "SET_DISPLAY",
  "data": {
    "page": "weather",
    "auto_rotate": false
  }
}
```
*Valid page names: `clock`, `weather`, `forecast`, `tasks`, `timer`, `alarm`, `status`.*

#### C. `SET_TASK` / `DELETE_TASK` / `TOGGLE_TASK`
```json
{
  "request_id": "c3",
  "command": "SET_TASK",
  "data": {
    "id": "t_99",
    "title": "Review client presentation",
    "priority": 3
  }
}
```

#### D. `SET_TIMER` / `START_TIMER` / `PAUSE_TIMER` / `STOP_TIMER`
```json
{
  "request_id": "c4",
  "command": "SET_TIMER",
  "data": {
    "duration": 1800
  }
}
```

#### E. `SET_ALARM` / `DELETE_ALARM` / `TOGGLE_ALARM`
```json
{
  "request_id": "c5",
  "command": "SET_ALARM",
  "data": {
    "id": "alm_01",
    "name": "Daily Standup",
    "hour": 9,
    "minute": 30,
    "enabled": true,
    "repeat_mask": 62
  }
}
```

#### F. System Commands
- `SYNC_TIME`: Trigger immediate NTP update.
- `REQUEST_STATUS`: Request immediate status publishing.
- `RESTART`: Soft reboot ESP32.
- `FACTORY_RESET`: Wipe configuration, restore factory defaults, and reboot.

---

## 4. Weather Ingestion (`.../weather`)

Publish normalized weather data directly to NEXORA:
```json
{
  "location": "Kathmandu",
  "temperature": 24.5,
  "feels_like": 25.1,
  "humidity": 68,
  "pressure": 1012,
  "wind_speed": 3.2,
  "condition": "Cloudy",
  "icon": "cloudy",
  "updated": 1757654400,
  "forecast": [
    {"day": "Sat", "temp_max": 26, "temp_min": 17, "condition": "Cloudy"},
    {"day": "Sun", "temp_max": 24, "temp_min": 16, "condition": "Rain"},
    {"day": "Mon", "temp_max": 25, "temp_min": 16, "condition": "Sunny"}
  ]
}
```

---

## 5. Device Events (`.../event`)

Published when hardware/application state triggers occur:
```json
{
  "device_id": "NEXORA-A1B2",
  "event": "TIMER_COMPLETE",
  "timestamp": 1757654400,
  "data": {
    "message": "Timer reached zero"
  }
}
```
```json
{
  "device_id": "NEXORA-A1B2",
  "event": "ALARM_TRIGGERED",
  "timestamp": 1757654400,
  "data": {
    "alarm_id": "alm_01"
  }
}
```
