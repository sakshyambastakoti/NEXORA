#include "OTAManager.h"
#include "Config.h"
#include "DeviceManager.h"
#include "DisplayManager.h"
#include "WiFiManager.h"
#include "StorageManager.h"

extern NEXORAConfig g_config;

OTAManager::OTAManager() : _server(80) {}

OTAManager& OTAManager::instance() {
    static OTAManager inst;
    return inst;
}

void OTAManager::begin() {
    if (_initialized) return;

    // 1. Initialize ArduinoOTA (for PlatformIO / espota upload on port 3232)
    ArduinoOTA.setHostname(g_config.deviceId);
    ArduinoOTA.setPassword(DEFAULT_OTA_PASSWORD);

    ArduinoOTA.onStart([this]() {
        _isUpdating = true;
        _lastProgressPct = -1;
        DeviceManager::instance().setState(DeviceState::OTA_UPDATE);
        Serial.println(F("[OTA] ArduinoOTA update process started."));
        DisplayManager::instance().showOTAScreen("STARTING FIRMWARE UPDATE...", 0, true);
    });

    ArduinoOTA.onEnd([]() {
        Serial.println(F("\n[OTA] Firmware update completed successfully! Rebooting..."));
        DisplayManager::instance().showOTAScreen("VERIFYING & REBOOTING...", 100, false);
    });

    ArduinoOTA.onProgress([this](unsigned int progress, unsigned int total) {
        int pct = (total > 0) ? (int)((progress * 100) / total) : 0;
        if (pct != _lastProgressPct) {
            _lastProgressPct = pct;
            DisplayManager::instance().showOTAScreen("RECEIVING FIRMWARE DATA...", pct, false);
            Serial.printf("[OTA] Progress: %d%%\r", pct);
        }
    });

    ArduinoOTA.onError([this](ota_error_t error) {
        _isUpdating = false;
        DeviceManager::instance().setState(DeviceState::ONLINE);
        Serial.printf("\n[OTA] Error [%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println(F("Auth Failed"));
        else if (error == OTA_BEGIN_ERROR) Serial.println(F("Begin Failed"));
        else if (error == OTA_CONNECT_ERROR) Serial.println(F("Connect Failed"));
        else if (error == OTA_RECEIVE_ERROR) Serial.println(F("Receive Failed"));
        else if (error == OTA_END_ERROR) Serial.println(F("End Failed"));
        DisplayManager::instance().showOTAScreen("OTA UPDATE FAILED!", 0, false);
    });

    ArduinoOTA.begin();

    // 2. Setup Web Server (for Web OTA and Network Configuration on port 80)
    setupWebServer();

    _initialized = true;
    Serial.println(F("[OTA] OTAManager (ArduinoOTA & Web Portal) initialized successfully."));
}

void OTAManager::update() {
    WiFiManager& wifi = WiFiManager::instance();
    // Run OTA whenever connected to Wi-Fi station OR when Fallback SoftAP is active
    if (wifi.isConnected() || wifi.isAPActive()) {
        if (!_initialized) begin();
        ArduinoOTA.handle();
        _server.handleClient();
    }
}

void OTAManager::setupWebServer() {
    const char* headerkeys[] = {"Content-Length"};
    size_t headerkeyssize = sizeof(headerkeys) / sizeof(char*);
    _server.collectHeaders(headerkeys, headerkeyssize);

    _server.on("/", HTTP_GET, [this]() { handleRoot(); });
    _server.on("/save-wifi", HTTP_POST, [this]() { handleSaveWifi(); });
    _server.on("/update", HTTP_GET, [this]() { handleUpdatePage(); });
    _server.on("/update", HTTP_POST,
        [this]() { handleUpdateDone(); },
        [this]() { handleUpdateUpload(); }
    );
    _server.onNotFound([this]() {
        _server.sendHeader("Location", "/");
        _server.send(302, "text/plain", "Redirecting to /");
    });

    _server.begin();
    Serial.println(F("[WEB] HTTP Portal server started on port 80."));
}

void OTAManager::handleRoot() {
    WiFiManager& wifi = WiFiManager::instance();
    DeviceManager& dev = DeviceManager::instance();

    String html;
    html.reserve(4096);

    html += F("<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1.0'>");
    html += F("<title>NEXORA Control & OTA Portal</title><style>");
    html += F("body{margin:0;padding:20px;background:#0d1117;color:#e6edf3;font-family:-apple-system,BlinkMacSystemFont,Segoe UI,Roboto,sans-serif;}");
    html += F(".container{max-width:540px;margin:0 auto;}");
    html += F(".header{text-align:center;padding:16px 0;border-bottom:1px solid #30363d;margin-bottom:20px;}");
    html += F(".header h1{margin:0;font-size:24px;color:#38bdf8;}");
    html += F(".header p{margin:4px 0 0;font-size:13px;color:#8b949e;}");
    html += F(".card{background:#161b22;border:1px solid #30363d;border-radius:10px;padding:18px;margin-bottom:18px;}");
    html += F(".card h2{margin:0 0 14px;font-size:16px;color:#f0f6fc;display:flex;align-items:center;gap:8px;}");
    html += F(".stat-row{display:flex;justify-content:space-between;padding:8px 0;border-bottom:1px solid #21262d;font-size:14px;}");
    html += F(".stat-row:last-child{border-bottom:none;}");
    html += F(".stat-label{color:#8b949e;}");
    html += F(".stat-val{font-weight:600;color:#e6edf3;}");
    html += F(".badge-ok{color:#3fb950;}.badge-warn{color:#e3b341;}");
    html += F("input[type=text],input[type=password]{width:100%;box-sizing:border-box;padding:10px;margin:6px 0 14px;background:#0d1117;border:1px solid #30363d;border-radius:6px;color:#fff;font-size:14px;}");
    html += F("button,.btn{display:block;width:100%;box-sizing:border-box;text-align:center;padding:12px;background:#0284c7;border:none;border-radius:6px;color:#fff;font-size:14px;font-weight:600;cursor:pointer;text-decoration:none;}");
    html += F("button:hover,.btn:hover{background:#0369a1;}");
    html += F(".btn-ota{background:#238636;margin-top:10px;}");
    html += F(".btn-ota:hover{background:#2ea043;}");
    html += F("</style></head><body><div class='container'>");

    // Header
    html += F("<div class='header'><h1>NEXORA</h1><p>Smart Information Station — System Portal</p></div>");

    // Status Card
    html += F("<div class='card'><h2>Status & Connectivity</h2>");
    html += F("<div class='stat-row'><span class='stat-label'>Device ID</span><span class='stat-val'>");
    html += g_config.deviceId;
    html += F("</span></div>");

    html += F("<div class='stat-row'><span class='stat-label'>Firmware Version</span><span class='stat-val'>");
    html += NEXORA_FIRMWARE_VERSION;
    html += F("</span></div>");

    html += F("<div class='stat-row'><span class='stat-label'>Station Wi-Fi</span><span class='stat-val'>");
    if (wifi.isConnected()) {
        html += F("<span class='badge-ok'>Connected</span> (");
        html += wifi.getSSID();
        html += F(")</span></div>");
        html += F("<div class='stat-row'><span class='stat-label'>Station IP</span><span class='stat-val'>");
        html += wifi.getIP();
        html += F("</span></div>");
    } else {
        html += F("<span class='badge-warn'>Connecting / Offline</span></span></div>");
    }

    html += F("<div class='stat-row'><span class='stat-label'>Fallback SoftAP</span><span class='stat-val'>");
    if (wifi.isAPActive()) {
        html += F("<span class='badge-ok'>ACTIVE (");
        html += wifi.getAPSSID();
        html += F(")</span></span></div>");
        html += F("<div class='stat-row'><span class='stat-label'>AP IP Address</span><span class='stat-val'>");
        html += wifi.getAPIP();
        html += F("</span></div>");
    } else {
        html += F("<span class='stat-label'>Inactive</span></span></div>");
    }

    html += F("<div class='stat-row'><span class='stat-label'>System Uptime</span><span class='stat-val'>");
    html += String(dev.getUptimeSeconds());
    html += F(" s</span></div>");

    html += F("<div class='stat-row'><span class='stat-label'>Free RAM</span><span class='stat-val'>");
    html += String(dev.getFreeHeap() / 1024);
    html += F(" KB</span></div></div>");

    // Wi-Fi Configuration Card
    html += F("<div class='card'><h2>Configure Wi-Fi Network</h2>");
    html += F("<form method='POST' action='/save-wifi'>");
    html += F("<label class='stat-label'>Wi-Fi SSID</label>");
    html += F("<input type='text' name='ssid' value='");
    html += g_config.wifiSsid;
    html += F("' placeholder='Enter Wi-Fi Network' required>");
    html += F("<label class='stat-label'>Wi-Fi Password</label>");
    html += F("<input type='password' name='password' value='");
    html += g_config.wifiPassword;
    html += F("' placeholder='Enter Password'>");
    html += F("<button type='submit'>Save Credentials & Connect</button>");
    html += F("</form></div>");

    // OTA Section Card
    html += F("<div class='card'><h2>Firmware Updates (OTA)</h2>");
    html += F("<p style='font-size:13px;color:#8b949e;margin:0 0 14px;'>Upload pre-compiled binary firmware directly to the clock or use PlatformIO OTA on port 3232.</p>");
    html += F("<a href='/update' class='btn btn-ota'>Open Firmware Update (Web OTA) &rarr;</a>");
    html += F("</div>");

    html += F("</div></body></html>");
    _server.send(200, "text/html", html);
}

void OTAManager::handleSaveWifi() {
    if (_server.hasArg("ssid")) {
        String newSsid = _server.arg("ssid");
        String newPass = _server.arg("password");

        strncpy(g_config.wifiSsid, newSsid.c_str(), sizeof(g_config.wifiSsid));
        strncpy(g_config.wifiPassword, newPass.c_str(), sizeof(g_config.wifiPassword));
        StorageManager::instance().saveConfig(g_config);

        String html;
        html.reserve(1024);
        html += F("<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1.0'>");
        html += F("<meta http-equiv='refresh' content='10;url=/'><style>");
        html += F("body{margin:0;padding:40px 20px;background:#0d1117;color:#e6edf3;font-family:-apple-system,sans-serif;text-align:center;}");
        html += F(".card{max-width:440px;margin:0 auto;background:#161b22;border:1px solid #30363d;border-radius:10px;padding:24px;}");
        html += F("h2{color:#3fb950;margin-top:0;}");
        html += F("p{color:#8b949e;font-size:14px;line-height:1.6;}");
        html += F("a{color:#38bdf8;text-decoration:none;}");
        html += F("</style></head><body><div class='card'>");
        html += F("<h2>Credentials Saved!</h2>");
        html += F("<p>NEXORA is now connecting to <strong>");
        html += newSsid;
        html += F("</strong>.<br>Redirecting to dashboard in 10 seconds...</p>");
        html += F("<p><a href='/'>Return to Dashboard immediately &rarr;</a></p>");
        html += F("</div></body></html>");

        _server.send(200, "text/html", html);
        delay(200);
        WiFiManager::instance().reconnect();
    } else {
        _server.send(400, "text/plain", "Missing SSID parameter.");
    }
}

void OTAManager::handleUpdatePage() {
    String html;
    html.reserve(2048);

    html += F("<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1.0'>");
    html += F("<title>NEXORA — Web OTA Update</title><style>");
    html += F("body{margin:0;padding:20px;background:#0d1117;color:#e6edf3;font-family:-apple-system,BlinkMacSystemFont,Segoe UI,Roboto,sans-serif;}");
    html += F(".container{max-width:480px;margin:0 auto;}");
    html += F(".header{text-align:center;padding:16px 0;border-bottom:1px solid #30363d;margin-bottom:20px;}");
    html += F(".header h1{margin:0;font-size:22px;color:#38bdf8;}");
    html += F(".card{background:#161b22;border:1px solid #30363d;border-radius:10px;padding:20px;}");
    html += F(".info{font-size:13px;color:#8b949e;margin-bottom:16px;line-height:1.5;}");
    html += F(".file-box{border:2px dashed #30363d;border-radius:8px;padding:20px;text-align:center;margin-bottom:16px;}");
    html += F("input[type=file]{color:#e6edf3;font-size:14px;}");
    html += F("button{display:block;width:100%;box-sizing:border-box;padding:12px;background:#238636;border:none;border-radius:6px;color:#fff;font-size:14px;font-weight:600;cursor:pointer;}");
    html += F("button:hover{background:#2ea043;}");
    html += F(".bar-wrap{display:none;background:#21262d;border-radius:6px;overflow:hidden;height:18px;margin-top:14px;}");
    html += F(".bar{background:#38bdf8;height:100%;width:0%;transition:width 0.2s;}");
    html += F(".back{display:inline-block;margin-top:16px;color:#38bdf8;text-decoration:none;font-size:14px;}");
    html += F("</style></head><body><div class='container'>");
    html += F("<div class='header'><h1>Firmware OTA Update</h1></div>");
    html += F("<div class='card'>");
    html += F("<p class='info'>Select a compiled <code>firmware.bin</code> file to flash directly to your clock over the network. The clock display will show a live progress bar during flashing.</p>");
    html += F("<form id='upForm' method='POST' action='/update' enctype='multipart/form-data'>");
    html += F("<div class='file-box'><input type='file' id='firmwareFile' name='update' accept='.bin' required></div>");
    html += F("<button type='submit' id='subBtn'>Flash Firmware to NEXORA</button>");
    html += F("<div class='bar-wrap' id='barWrap'><div class='bar' id='bar'></div></div>");
    html += F("</form>");
    html += F("<a href='/' class='back'>&larr; Back to System Dashboard</a>");
    html += F("</div></div>");
    html += F("<script>");
    html += F("const form=document.getElementById('upForm');const wrap=document.getElementById('barWrap');const bar=document.getElementById('bar');const btn=document.getElementById('subBtn');");
    html += F("form.onsubmit=function(e){");
    html += F("e.preventDefault();");
    html += F("const fileInput=document.getElementById('firmwareFile');");
    html += F("if(!fileInput.files.length)return;");
    html += F("btn.disabled=true;btn.innerText='Flashing In Progress...';wrap.style.display='block';");
    html += F("const xhr=new XMLHttpRequest();xhr.open('POST','/update');");
    html += F("xhr.upload.onprogress=function(e){if(e.lengthComputable){const p=Math.round((e.loaded/e.total)*100);bar.style.width=p+'%';bar.innerText=p+'%';}};");
    html += F("xhr.onload=function(){if(xhr.status==200){document.body.innerHTML=xhr.responseText;}else{alert('Update failed: '+xhr.statusText);btn.disabled=false;btn.innerText='Flash Firmware';}};");
    html += F("const fd=new FormData();fd.append('update',fileInput.files[0]);xhr.send(fd);");
    html += F("};</script></body></html>");

    _server.send(200, "text/html", html);
}

void OTAManager::handleUpdateUpload() {
    HTTPUpload& upload = _server.upload();

    if (upload.status == UPLOAD_FILE_START) {
        _isUpdating = true;
        _lastProgressPct = -1;
        _uploadTotalSize = 0;
        if (_server.hasHeader("Content-Length")) {
            _uploadTotalSize = _server.header("Content-Length").toInt();
        }
        DeviceManager::instance().setState(DeviceState::OTA_UPDATE);
        Serial.printf("[WEB-OTA] Starting update: %s (Total size: %u bytes)\n", upload.filename.c_str(), (unsigned int)_uploadTotalSize);
        DisplayManager::instance().showOTAScreen("STARTING FIRMWARE FLASH...", 0, true);

        size_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
        if (!Update.begin(maxSketchSpace, U_FLASH)) {
            Update.printError(Serial);
            DisplayManager::instance().showOTAScreen("FLASH INIT FAILED!", 0, false);
            _isUpdating = false;
            DeviceManager::instance().setState(DeviceState::ONLINE);
        }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (!_isUpdating) return;

        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
            Update.printError(Serial);
            DisplayManager::instance().showOTAScreen("FLASH WRITE ERROR!", 0, false);
            _isUpdating = false;
            DeviceManager::instance().setState(DeviceState::ONLINE);
            return;
        }

        int pct = 50;
        if (_uploadTotalSize > 0) {
            pct = (int)((upload.totalSize * 100) / _uploadTotalSize);
            if (pct > 99) pct = 99; // 100% reserved for end
        }

        if (pct != _lastProgressPct) {
            _lastProgressPct = pct;
            DisplayManager::instance().showOTAScreen("FLASHING FIRMWARE DATA...", pct, false);
            Serial.printf("[WEB-OTA] Progress: %d%%\r", pct);
        }
    } else if (upload.status == UPLOAD_FILE_END) {
        if (!_isUpdating) return;

        if (Update.end(true)) {
            Serial.printf("\n[WEB-OTA] Flash successful: %u bytes written! Rebooting...\n", (unsigned int)upload.totalSize);
            DisplayManager::instance().showOTAScreen("VERIFYING & REBOOTING...", 100, false);
        } else {
            Update.printError(Serial);
            DisplayManager::instance().showOTAScreen("OTA VALIDATION FAILED!", 0, false);
            _isUpdating = false;
            DeviceManager::instance().setState(DeviceState::ONLINE);
        }
    } else if (upload.status == UPLOAD_FILE_ABORTED) {
        Update.end();
        Serial.println(F("\n[WEB-OTA] Update aborted by client."));
        DisplayManager::instance().showOTAScreen("UPDATE ABORTED!", 0, false);
        _isUpdating = false;
        DeviceManager::instance().setState(DeviceState::ONLINE);
    }
}

void OTAManager::handleUpdateDone() {
    bool ok = !Update.hasError() && _isUpdating;
    String html;
    html.reserve(1024);
    html += F("<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1.0'>");
    html += F("<style>body{margin:0;padding:40px 20px;background:#0d1117;color:#e6edf3;font-family:-apple-system,sans-serif;text-align:center;}");
    html += F(".card{max-width:440px;margin:0 auto;background:#161b22;border:1px solid #30363d;border-radius:10px;padding:24px;}");
    html += F("h2{color:#3fb950;margin-top:0;}</style></head><body><div class='card'>");

    if (ok) {
        html += F("<h2>Update Complete!</h2><p>Firmware successfully flashed. NEXORA is rebooting now...</p>");
    } else {
        html += F("<h2 style='color:#f85149;'>Update Failed</h2><p>The firmware could not be verified or written. Please check the file and try again.</p><p><a href='/update' style='color:#38bdf8;'>Try Again &rarr;</a></p>");
    }

    html += F("</div></body></html>");
    _server.sendHeader("Connection", "close");
    _server.send(ok ? 200 : 500, "text/html", html);

    if (ok) {
        delay(1500);
        ESP.restart();
    }
}


