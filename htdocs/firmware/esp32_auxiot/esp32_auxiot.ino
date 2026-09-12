#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "Audio.h"

// AuxIoT ESP32-S3 reference firmware.
// Configure these for your installation or replace with a provisioning flow.
static const char *WIFI_SSID = "YOUR_WIFI";
static const char *WIFI_PASS = "YOUR_PASSWORD";
static const char *STATE_URL = "https://aiotamp.rf.gd/api.php?action=state";
static const char *STATUS_URL = "https://aiotamp.rf.gd/api.php?action=device_status";
static const char *DEVICE_ID = "auxiot-esp32-s3-01";
static const char *FIRMWARE_VERSION = "1.0.0";

static const int I2S_BCLK = 7;
static const int I2S_LRC = 6;
static const int I2S_DOUT = 5;

Audio audio;
Preferences preferences;

uint32_t lastCommandId = 0;
uint32_t lastPollMs = 0;
uint32_t lastStatusMs = 0;
String currentAction = "stop";
String currentTitle = "";
String currentUrl = "";
String lastError = "";
uint8_t currentVolume = 12;

void stopSong() {
    audio.stopSong();
    currentAction = "stop";
    currentTitle = "";
    currentUrl = "";
}

void connectWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    uint32_t started = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - started < 15000) {
        delay(250);
    }
}

bool getJson(const char *url, JsonDocument &doc) {
    if (WiFi.status() != WL_CONNECTED) {
        connectWiFi();
    }
    if (WiFi.status() != WL_CONNECTED) {
        lastError = "WiFi disconnected";
        return false;
    }

    HTTPClient http;
    http.setReuse(false);
    http.begin(url);
    http.addHeader("Cache-Control", "no-cache");
    int code = http.GET();
    if (code != 200) {
        lastError = "GET failed: " + String(code);
        http.end();
        return false;
    }

    DeserializationError error = deserializeJson(doc, http.getStream());
    http.end();
    if (error) {
        lastError = "JSON parse failed";
        return false;
    }
    return true;
}

void executeCommand(JsonObject state) {
    uint32_t commandId = state["command_id"] | 0;
    if (commandId == 0 || commandId == lastCommandId) {
        return;
    }
    lastCommandId = commandId;

    String action = state["action"] | "stop";
    String url = state["url"] | "";
    String title = state["title"] | "";
    int volume = state["volume"] | currentVolume;
    volume = constrain(volume, 0, 21);
    audio.setVolume(volume);
    currentVolume = volume;

    if (action == "play") {
        if (!url.startsWith("http://") && !url.startsWith("https://") && !url.startsWith("/")) {
            lastError = "Invalid stream URL";
            stopSong();
            return;
        }
        stopSong();
        delay(180);
        currentAction = "play";
        currentTitle = title;
        currentUrl = url;
        if (!audio.connecttohost(url.c_str())) {
            lastError = "Unable to open stream";
            stopSong();
        }
        return;
    }

    if (action == "pause" || action == "stop") {
        stopSong();
        return;
    }

    if (action == "volume") {
        currentAction = "volume";
        return;
    }

    if (action == "soft_reset") {
        stopSong();
        delay(100);
        ESP.restart();
    }

    if (action == "factory_reset") {
        stopSong();
        preferences.begin("auxiot", false);
        preferences.clear();
        preferences.end();
        WiFi.disconnect(true, true);
        delay(250);
        ESP.restart();
    }
}

void pollState() {
    StaticJsonDocument<1536> doc;
    if (!getJson(STATE_URL, doc)) {
        return;
    }
    if (!(doc["ok"] | false)) {
        lastError = "State API returned error";
        return;
    }
    executeCommand(doc["state"].as<JsonObject>());
}

void sendStatus() {
    if (WiFi.status() != WL_CONNECTED) {
        connectWiFi();
    }
    if (WiFi.status() != WL_CONNECTED) {
        return;
    }

    StaticJsonDocument<1536> doc;
    doc["device_id"] = DEVICE_ID;
    doc["ip_address"] = WiFi.localIP().toString();
    doc["wifi_status"] = WiFi.status() == WL_CONNECTED ? "connected" : "disconnected";
    doc["rssi"] = WiFi.RSSI();
    doc["free_heap"] = ESP.getFreeHeap();
    doc["psram_status"] = psramFound() ? "available" : "missing";
    doc["command_id"] = lastCommandId;
    doc["action"] = currentAction;
    doc["track_title"] = currentTitle;
    doc["stream_url"] = currentUrl;
    doc["volume"] = currentVolume;
    doc["playback_state"] = audio.isRunning() ? "playing" : "stopped";
    doc["last_error"] = lastError;
    doc["firmware_version"] = FIRMWARE_VERSION;
    doc["uptime_ms"] = millis();

    String body;
    serializeJson(doc, body);

    HTTPClient http;
    http.setReuse(false);
    http.begin(STATUS_URL);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Cache-Control", "no-cache");
    http.POST(body);
    http.end();
}

void setup() {
    Serial.begin(115200);
    connectWiFi();
    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio.setVolume(currentVolume);
}

void loop() {
    audio.loop();

    if (millis() - lastPollMs >= 2000) {
        lastPollMs = millis();
        pollState();
    }

    if (millis() - lastStatusMs >= 2000) {
        lastStatusMs = millis();
        sendStatus();
    }
}
