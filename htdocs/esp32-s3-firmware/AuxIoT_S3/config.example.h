#pragma once

// Copy this file to config.h, then edit values for your board and server.

#define AUXIOT_DEVICE_ID "auxiot-esp32-s3-01"
#define AUXIOT_FIRMWARE_VERSION "1.0.0"

// InfinityFree currently adds a browser JavaScript challenge on free domains.
// The ESP32 cannot solve that challenge. Keep these URLs here for the server
// contract, but use a challenge-free API host for reliable hardware polling.
#define AUXIOT_BASE_URL "https://aiotamp.rf.gd"
#define AUXIOT_STREAM_BASE_URL "https://aiotamp.rf.gd"

// InfinityFree browser challenge cookie observed for aiotamp.rf.gd.
// If the host rotates this value, the ESP32 will stop receiving JSON again.
#define AUXIOT_INFINITYFREE_COOKIE "__test=fe87e22394447e26c9569d45f84c4cd7"

// First-run WiFi provisioning access point.
// Connect phone/laptop to SSID auxIoT with password password, then open
// http://192.168.4.1 and save your real WiFi credentials.
#define AUXIOT_PROVISIONING_SSID "auxIoT"
#define AUXIOT_PROVISIONING_PASS "password"

// Optional fallback WiFi credentials. If left blank, send this over Serial:
// WIFI your-ssid your-password
#define AUXIOT_FALLBACK_WIFI_SSID ""
#define AUXIOT_FALLBACK_WIFI_PASS ""

// PCM5102A I2S pins. Change these to match your wiring.
#define AUXIOT_I2S_BCLK 12
#define AUXIOT_I2S_LRC 13
#define AUXIOT_I2S_DOUT 11

// Audio library volume range is 0-21.
#define AUXIOT_DEFAULT_VOLUME 12
