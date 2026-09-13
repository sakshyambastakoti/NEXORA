# NEXORA Firmware Guide

## Development Environment
- **IDE / Tooling**: PlatformIO Core (`pio`) or VS Code PlatformIO IDE
- **Target Microcontroller**: Arduino Nano ESP32 (ESP32-S3)
- **Display**: 3.5" ILI9488 TFT LCD (320x480 native, 480x320 landscape)
- **Display Interface**: 8-Bit Parallel Bus

---

## 1. Hardware Pinout (8-Bit Parallel)

All GPIO assignments strictly follow `connection.md`.

```
========================================================================================
Display Pin       Nano ESP32 Silk Pin     ESP32-S3 Hardware GPIO    Function
========================================================================================
VCC / 5V          VBUS / 5V               —                         5V Power Supply
GND               GND                     —                         Common Ground
LCD_RD            A0                      GPIO 1                    Read Strobe
LCD_WR            A1                      GPIO 2                    Write Strobe
LCD_RS / DC       A2                      GPIO 3                    Command/Data Select
LCD_CS            A3                      GPIO 4                    Chip Select (Active LOW)
LCD_RST           A4                      GPIO 11                   Reset (Active LOW)
LCD_D0            D2                      GPIO 5                    Bus Bit 0
LCD_D1            D3                      GPIO 6                    Bus Bit 1
LCD_D2            D4                      GPIO 7                    Bus Bit 2
LCD_D3            D5                      GPIO 8                    Bus Bit 3
LCD_D4            D6                      GPIO 9                    Bus Bit 4
LCD_D5            D7                      GPIO 10                   Bus Bit 5
LCD_D6            D8                      GPIO 17                   Bus Bit 6
LCD_D7            D9                      GPIO 18                   Bus Bit 7
========================================================================================
```

> **Crucial Setting**: In `platformio.ini`, `-D BOARD_USES_HW_GPIO_NUMBERS` is mandatory so pins match ESP32-S3 native GPIO indices.

---

## 2. Configuration & Secrets

1. Copy `include/secrets.h.template` to `include/secrets.h`:
   ```bash
   cp include/secrets.h.template include/secrets.h
   ```
2. Edit `include/secrets.h` with your local Wi-Fi SSID, password, and MQTT broker details.

---

## 3. Building & Flashing

### Compile Firmware
```bash
pio run
```

### Flash via USB Serial
Connect the Arduino Nano ESP32 via USB-C and run:
```bash
pio run --target upload
```

### Open Serial Monitor
```bash
pio device monitor -b 115200
```

---

## 4. OTA (Over-The-Air) Updates & Web Portal

NEXORA supports firmware updates and device configuration over **both** your local Wi-Fi and its built-in **Fallback SoftAP**.

### Option A: Web Browser OTA & Portal (Easiest)
1. If connected to Station Wi-Fi (`sakshyam`), open your browser to `http://<STATION_IP>/`.
2. If in Fallback AP mode, connect your phone or laptop to Wi-Fi SSID **`NEXORA-{DEVICE_ID}-AP`** (Password: `nexora1234`) and browse to:
   - **System Dashboard & Wi-Fi Config**: `http://192.168.4.1/`
   - **Direct Web OTA Upload**: `http://192.168.4.1/update`
3. Click "Choose File", select `.pio/build/nano_esp32/firmware.bin`, and click **Flash Firmware**.
4. The clock display will show a live progress bar and automatically reboot when complete.

### Option B: PlatformIO ArduinoOTA (Port 3232)
NEXORA advertises the ArduinoOTA service:
- **Hostname**: `NEXORA-{DEVICE_ID}` (e.g. `NEXORA-A1B2`)
- **Port**: `3232` (Default ESP32 OTA)
- **Password**: Configured in `include/secrets.h` (`DEFAULT_OTA_PASSWORD`, default: `nexora_admin_ota`)

To flash over the network via PlatformIO:
```bash
pio run -t upload --upload-port <DEVICE_IP_OR_192.168.4.1> --upload-flags "--auth=nexora_admin_ota"
```


---

## 5. Serial Log Output Example

```text
==================================================
  NEXORA — Smart Personal Information Station     
  Production Firmware v1.0.0 (Arduino Nano ESP32) 
==================================================
[STORAGE] Initializing NVS storage...
[STORAGE] Config loaded successfully for device: NEXORA-9F4A
[DISPLAY] Initializing 3.5" ILI9488 TFT LCD in 8-Bit Parallel Mode...
[DISPLAY] ILI9488 initialized successfully.
[DEVICE] DeviceManager initialized.
[TIME] Initializing TimeManager...
[TIME] Setting timezone POSIX string: <+0545>-5:45
[WEATHER] Initializing WeatherManager...
[WEATHER] Loading cached weather from storage...
[TASKS] Initializing TaskManager...
[TIMER] Initializing TimerManager...
[ALARM] Initializing AlarmManager...
[WIFI] Initializing Wi-Fi station...
[WIFI] Connecting to SSID: HomeNetwork
[MQTT] Initializing MQTT Manager...
[UI] Initializing UIManager...
[BOOT] NEXORA boot sequence completed successfully.
[WIFI] Connected! IP: 192.168.1.142 | RSSI: -46 dBm
[TIME] Triggering NTP time synchronization...
[TIME] NTP synchronized successfully! Current Epoch: 1757654400
[MQTT] Connecting to broker broker.hivemq.com:1883 as NEXORA-9F4A...
[MQTT] Connected to broker successfully!
[MQTT] Subscribed to nexora/NEXORA-9F4A/command, nexora/NEXORA-9F4A/config, nexora/NEXORA-9F4A/weather
[MQTT] Published status telemetry.
```
