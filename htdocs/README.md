# AuxIoT

AuxIoT is a self-hosted PWA and ESP32-S3 IoT audio controller. The server is the source of truth; the ESP32 is a stateless executor that polls commands, ignores repeated `command_id` values, plays one stream at a time, and reports Inner Eye telemetry.

## Setup

1. Create a MySQL database named `auxiot`.
2. Import [database/schema.sql](database/schema.sql).
3. On InfinityFree, copy `config/local.example.php` to `config/local.php` and fill in the MySQL host, database name, username, and password from the control panel.
4. Update database credentials through environment variables when needed:
   - `AUXIOT_DB_DSN`
   - `AUXIOT_DB_USER`
   - `AUXIOT_DB_PASS`
5. Serve the project with PHP/XAMPP.
6. Open `index.php` and install the PWA from the browser.

## InfinityFree Deployment

Upload the app contents into the site `htdocs` folder for `https://aiotamp.rf.gd/`. The generated deployment package is `dist/auxiot-aiotamp-rf-gd.zip`; extract its contents directly into `htdocs`, not into a nested subfolder.

## Required Runtime Paths

- `storage/state.json` stores the atomic playback command state.
- `storage/state.lock` is created automatically for state and reset locking.
- `uploads/` stores validated MP3 uploads.

## API

All dynamic endpoints return JSON and send `Cache-Control: no-store, no-cache, must-revalidate, max-age=0`.

- `GET api.php?action=state`
- `GET api.php?action=library`
- `GET api.php?action=playlists`
- `GET api.php?action=device_status`
- `POST api.php?action=control`
- `POST api.php?action=upload`
- `POST api.php?action=add_link`
- `POST api.php?action=delete_track`
- `POST api.php?action=playlist_create`
- `POST api.php?action=playlist_add`
- `POST api.php?action=playlist_remove`
- `POST api.php?action=play_playlist`
- `POST api.php?action=device_status`
- `POST api.php?action=reset`

## Reset Types

- `playback`: stop playback and clear active stream state.
- `soft_device`: issue an ESP32 restart command.
- `factory_device`: issue ESP32 factory reset command. Requires `confirm=RESET`.
- `server`: reinitialize `state.json`.
- `library`: delete tracks, playlists, and uploaded files. Requires `confirm=RESET`.
- `full`: reset state, library, uploads, playlists, and device status. Requires `confirm=RESET`.

## Firmware

The reference ESP32-S3 firmware lives in [firmware/esp32_auxiot/esp32_auxiot.ino](firmware/esp32_auxiot/esp32_auxiot.ino). Configure WiFi, server URLs, and I2S pins before flashing.
