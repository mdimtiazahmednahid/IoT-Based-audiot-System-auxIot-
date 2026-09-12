CREATE TABLE IF NOT EXISTS tracks (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    title VARCHAR(255) NOT NULL,
    source_type ENUM('upload', 'link') NOT NULL,
    file_url TEXT NOT NULL,
    file_path TEXT NULL,
    mime_type VARCHAR(120) NOT NULL,
    duration_seconds INT UNSIGNED NULL,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS playlists (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS playlist_tracks (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    playlist_id INT UNSIGNED NOT NULL,
    track_id INT UNSIGNED NOT NULL,
    sort_order INT UNSIGNED NOT NULL DEFAULT 0,
    CONSTRAINT fk_playlist_tracks_playlist FOREIGN KEY (playlist_id) REFERENCES playlists(id) ON DELETE CASCADE,
    CONSTRAINT fk_playlist_tracks_track FOREIGN KEY (track_id) REFERENCES tracks(id) ON DELETE CASCADE,
    INDEX idx_playlist_tracks_playlist_order (playlist_id, sort_order)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS device_status (
    device_id VARCHAR(120) PRIMARY KEY,
    ip_address VARCHAR(64) NULL,
    wifi_status VARCHAR(40) NULL,
    rssi INT NULL,
    free_heap INT UNSIGNED NULL,
    psram_status VARCHAR(80) NULL,
    command_id INT UNSIGNED NOT NULL DEFAULT 0,
    action VARCHAR(40) NULL,
    track_title VARCHAR(255) NULL,
    stream_url TEXT NULL,
    volume TINYINT UNSIGNED NOT NULL DEFAULT 12,
    playback_state VARCHAR(40) NULL,
    last_error TEXT NULL,
    firmware_version VARCHAR(80) NULL,
    uptime_ms BIGINT UNSIGNED NULL,
    updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS users (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    email VARCHAR(255) NOT NULL UNIQUE,
    passcode_hash VARCHAR(255) NOT NULL,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
