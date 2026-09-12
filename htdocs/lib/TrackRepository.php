<?php

declare(strict_types=1);

final class TrackRepository
{
    private static bool $schemaReady = false;

    public function all(): array
    {
        $this->ensureSchema();
        $tracks = Database::pdo()->query('SELECT * FROM tracks ORDER BY created_at DESC, id DESC')->fetchAll();
        foreach ($tracks as &$track) {
            $track = $this->withDuration($track);
        }
        return $tracks;
    }

    public function find(int $id): ?array
    {
        $this->ensureSchema();
        $stmt = Database::pdo()->prepare('SELECT * FROM tracks WHERE id = ?');
        $stmt->execute([$id]);
        $track = $stmt->fetch();
        return $track ? $this->withDuration($track) : null;
    }

    public function createLink(string $title, string $url): array
    {
        $this->ensureSchema();
        $stmt = Database::pdo()->prepare('INSERT INTO tracks (title, source_type, file_url, file_path, mime_type, duration_seconds) VALUES (?, "link", ?, NULL, "audio/mpeg", NULL)');
        $stmt->execute([$title, $url]);
        return $this->find((int) Database::pdo()->lastInsertId());
    }

    public function createUpload(string $title, string $url, string $path, string $mime): array
    {
        $this->ensureSchema();
        $duration = $this->mp3DurationSeconds($path);
        $stmt = Database::pdo()->prepare('INSERT INTO tracks (title, source_type, file_url, file_path, mime_type, duration_seconds) VALUES (?, "upload", ?, ?, ?, ?)');
        $stmt->execute([$title, $url, $path, $mime, $duration]);
        return $this->find((int) Database::pdo()->lastInsertId());
    }

    public function rename(int $id, string $title): ?array
    {
        $this->ensureSchema();
        $stmt = Database::pdo()->prepare('UPDATE tracks SET title = ? WHERE id = ?');
        $stmt->execute([$title, $id]);
        return $this->find($id);
    }

    public function delete(int $id): void
    {
        $track = $this->find($id);
        if (!$track) {
            return;
        }

        if ($track['source_type'] === 'upload' && $track['file_path']) {
            $this->deleteUploadFile($track['file_path']);
        }

        $stmt = Database::pdo()->prepare('DELETE FROM tracks WHERE id = ?');
        $stmt->execute([$id]);
    }

    public function deleteAllUploads(): void
    {
        foreach ($this->all() as $track) {
            if ($track['source_type'] === 'upload' && $track['file_path']) {
                $this->deleteUploadFile($track['file_path']);
            }
        }

        $uploads = realpath(auxiot_config()['paths']['uploads']);
        if ($uploads === false) {
            return;
        }
        foreach (glob($uploads . DIRECTORY_SEPARATOR . '*.mp3') ?: [] as $file) {
            $this->deleteUploadFile($file);
        }
    }

    private function deleteUploadFile(string $path): void
    {
        $uploads = realpath(auxiot_config()['paths']['uploads']);
        $target = realpath($path);
        if ($uploads === false) {
            throw new RuntimeException('Upload directory is unavailable.');
        }
        if ($target === false) {
            $candidate = dirname($path);
            $parent = realpath($candidate);
            if ($parent === false || $parent !== $uploads) {
                throw new RuntimeException('Unsafe upload path refused.');
            }
            return;
        }
        if (!str_starts_with($target, $uploads . DIRECTORY_SEPARATOR)) {
            throw new RuntimeException('Unsafe upload path refused.');
        }
        if (is_file($target)) {
            unlink($target);
        }
    }

    private function ensureSchema(): void
    {
        if (self::$schemaReady) {
            return;
        }

        $stmt = Database::pdo()->query("SHOW COLUMNS FROM tracks LIKE 'duration_seconds'");
        if (!$stmt->fetch()) {
            Database::pdo()->exec('ALTER TABLE tracks ADD COLUMN duration_seconds INT UNSIGNED NULL AFTER mime_type');
        }
        self::$schemaReady = true;
    }

    private function withDuration(array $track): array
    {
        if (($track['source_type'] ?? '') !== 'upload' || !empty($track['duration_seconds']) || empty($track['file_path'])) {
            return $track;
        }

        $duration = $this->mp3DurationSeconds((string) $track['file_path']);
        if ($duration !== null) {
            $stmt = Database::pdo()->prepare('UPDATE tracks SET duration_seconds = ? WHERE id = ?');
            $stmt->execute([$duration, (int) $track['id']]);
            $track['duration_seconds'] = $duration;
        }
        return $track;
    }

    private function mp3DurationSeconds(string $path): ?int
    {
        $real = realpath($path);
        if ($real === false || !is_file($real)) {
            return null;
        }

        $data = file_get_contents($real);
        if ($data === false || strlen($data) < 4) {
            return null;
        }

        $length = strlen($data);
        $pos = 0;
        if (substr($data, 0, 3) === 'ID3' && $length >= 10) {
            $pos = 10 + ((ord($data[6]) & 0x7f) << 21) + ((ord($data[7]) & 0x7f) << 14) + ((ord($data[8]) & 0x7f) << 7) + (ord($data[9]) & 0x7f);
        }

        $seconds = 0.0;
        $frames = 0;
        while ($pos + 4 <= $length) {
            $b1 = ord($data[$pos]);
            $b2 = ord($data[$pos + 1]);
            $b3 = ord($data[$pos + 2]);
            $b4 = ord($data[$pos + 3]);

            if ($b1 !== 0xff || (($b2 & 0xe0) !== 0xe0)) {
                $pos++;
                continue;
            }

            $versionBits = ($b2 >> 3) & 0x03;
            $layerBits = ($b2 >> 1) & 0x03;
            $bitrateIndex = ($b3 >> 4) & 0x0f;
            $sampleIndex = ($b3 >> 2) & 0x03;
            $padding = ($b3 >> 1) & 0x01;
            if ($versionBits === 1 || $layerBits === 0 || $bitrateIndex === 0 || $bitrateIndex === 15 || $sampleIndex === 3) {
                $pos++;
                continue;
            }

            $isMpeg1 = $versionBits === 3;
            $layer = 4 - $layerBits;
            $sampleRates = $versionBits === 3 ? [44100, 48000, 32000] : ($versionBits === 2 ? [22050, 24000, 16000] : [11025, 12000, 8000]);
            $sampleRate = $sampleRates[$sampleIndex];
            $bitrate = $this->mp3Bitrate($isMpeg1, $layer, $bitrateIndex);
            if ($bitrate < 1) {
                $pos++;
                continue;
            }

            $samples = $layer === 1 ? 384 : ($layer === 3 && !$isMpeg1 ? 576 : 1152);
            if ($layer === 1) {
                $frameLength = (int) floor(((12 * $bitrate * 1000 / $sampleRate) + $padding) * 4);
            } elseif ($layer === 3 && !$isMpeg1) {
                $frameLength = (int) floor((72 * $bitrate * 1000 / $sampleRate) + $padding);
            } else {
                $frameLength = (int) floor((144 * $bitrate * 1000 / $sampleRate) + $padding);
            }
            if ($frameLength < 4) {
                $pos++;
                continue;
            }

            $seconds += $samples / $sampleRate;
            $frames++;
            $pos += $frameLength;
        }

        return $frames > 0 ? max(1, (int) round($seconds)) : null;
    }

    private function mp3Bitrate(bool $mpeg1, int $layer, int $index): int
    {
        $tables = $mpeg1
            ? [
                1 => [0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448],
                2 => [0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384],
                3 => [0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320],
            ]
            : [
                1 => [0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256],
                2 => [0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160],
                3 => [0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160],
            ];
        return $tables[$layer][$index] ?? 0;
    }
}
