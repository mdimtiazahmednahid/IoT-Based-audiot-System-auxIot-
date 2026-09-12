# AuxIoT ESP32-S3 Firmware

This is the complete ESP32-S3 firmware project for AuxIoT.

## Arduino Libraries

Install these libraries in Arduino IDE:

- `ArduinoJson`
- `ESP32-audioI2S` by schreibfaul1

The ESP32 board package already provides:

- `WiFi`
- `HTTPClient`
- `WiFiClientSecure`
- `Preferences`

## Setup

1. Copy `config.example.h` to `config.h`.
2. Edit `config.h` for your I2S pins, device ID, and API base URL.
3. Flash `AuxIoT_S3.ino` to the ESP32-S3.
4. Open Serial Monitor at `115200`.
5. If no WiFi credentials are stored, send:

```text
WIFI your-ssid your-password
```

The device stores WiFi credentials in ESP32 preferences. A factory reset command clears them.

## Hardware Contract

- Polls `GET /api.php?action=state` every 2 seconds.
- Executes a command only when `command_id` changes.
- Stops the current stream before starting a new stream.
- Reduces API polling while audio is playing to avoid stream buffering.
- Sends the InfinityFree challenge cookie as an audio request header so uploaded `/uploads/*.mp3` files can play from the hosted site.
- Uses the verified PCM5102A pins from the working test sketch by default:
  - BCLK `12`
  - LRC `13`
  - DOUT `11`
- Sends status to `POST /api.php?action=device_status` every 2 seconds.
- Handles:
  - `play`
  - `pause` as stop
  - `stop`
  - `volume`
  - `soft_reset`
  - `factory_reset`

## Serial Commands

The firmware keeps the working serial test commands:

```text
paste URL then Enter
vol 0-21
stop
status
WIFI your-ssid your-password
```

It also keeps the `ESP32-audioI2S` debug callbacks, including stream info, station, title, bitrate, ID3, EOF, and last host messages.

## Important InfinityFree Note

InfinityFree free domains can return a browser JavaScript cookie challenge before PHP. Browsers pass it, but ESP32 firmware cannot run that JavaScript. If the ESP32 does not appear online, move the API to a host without that challenge or add a separate challenge-free proxy endpoint.

This installed build patches `ESP32-audioI2S-master` locally with `Audio::setCustomHTTPHeader(...)` so uploaded files can include the InfinityFree cookie header during playback. A normal upstream copy of that library will need the same small header patch before compiling this sketch.
