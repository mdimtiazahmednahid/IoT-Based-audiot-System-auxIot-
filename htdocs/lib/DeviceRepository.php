<?php

declare(strict_types=1);

final class DeviceRepository
{
    public function all(): array
    {
        $devices = Database::pdo()->query('SELECT device_status.*, TIMESTAMPDIFF(SECOND, updated_at, NOW()) AS age_seconds FROM device_status ORDER BY updated_at DESC')->fetchAll();
        foreach ($devices as &$device) {
            $age = max(0, (int) $device['age_seconds']);
            $isPlaying = ($device['playback_state'] ?? '') === 'playing' || ($device['action'] ?? '') === 'play';
            $onlineLimit = $isPlaying ? 43200 : 10;
            $staleLimit = $isPlaying ? 86400 : 30;
            $device['online_state'] = $age < $onlineLimit ? 'online' : ($age <= $staleLimit ? 'stale' : 'offline');
            $device['age_seconds'] = $age;
        }
        return $devices;
    }

    public function upsert(array $payload): void
    {
        $fields = [
            'device_id', 'ip_address', 'wifi_status', 'rssi', 'free_heap', 'psram_status',
            'command_id', 'action', 'track_title', 'stream_url', 'volume', 'playback_state',
            'last_error', 'firmware_version', 'uptime_ms',
        ];
        $data = [];
        foreach ($fields as $field) {
            $data[$field] = $payload[$field] ?? null;
        }
        if (!$data['device_id']) {
            throw new InvalidArgumentException('device_id is required.');
        }
        $data['command_id'] = (int) ($data['command_id'] ?? 0);
        $data['volume'] = (int) ($data['volume'] ?? 12);
        $stmt = Database::pdo()->prepare(
            'INSERT INTO device_status (device_id, ip_address, wifi_status, rssi, free_heap, psram_status, command_id, action, track_title, stream_url, volume, playback_state, last_error, firmware_version, uptime_ms)
             VALUES (:device_id, :ip_address, :wifi_status, :rssi, :free_heap, :psram_status, :command_id, :action, :track_title, :stream_url, :volume, :playback_state, :last_error, :firmware_version, :uptime_ms)
             ON DUPLICATE KEY UPDATE ip_address=VALUES(ip_address), wifi_status=VALUES(wifi_status), rssi=VALUES(rssi), free_heap=VALUES(free_heap), psram_status=VALUES(psram_status), command_id=VALUES(command_id), action=VALUES(action), track_title=VALUES(track_title), stream_url=VALUES(stream_url), volume=VALUES(volume), playback_state=VALUES(playback_state), last_error=VALUES(last_error), firmware_version=VALUES(firmware_version), uptime_ms=VALUES(uptime_ms), updated_at=CURRENT_TIMESTAMP'
        );
        $stmt->execute($data);
    }

    public function clear(): void
    {
        Database::pdo()->exec('DELETE FROM device_status');
    }
}
