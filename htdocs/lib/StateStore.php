<?php

declare(strict_types=1);

final class StateStore
{
    public function defaultState(): array
    {
        return [
            'command_id' => 0,
            'action' => 'stop',
            'url' => '',
            'title' => '',
            'volume' => 12,
            'playlist_id' => null,
            'track_id' => null,
            'duration_seconds' => null,
            'auto_play' => false,
            'is_playing' => false,
            'playback_index' => 0,
            'updated_at' => gmdate('c'),
        ];
    }

    public function read(): array
    {
        $path = auxiot_config()['paths']['state'];
        if (!is_file($path)) {
            $this->write($this->defaultState());
        }

        $raw = file_get_contents($path);
        $decoded = json_decode($raw ?: '', true);
        return is_array($decoded) ? array_merge($this->defaultState(), $decoded) : $this->defaultState();
    }

    public function command(array $changes): array
    {
        return $this->locked(function () use ($changes): array {
            $state = $this->read();
            $state = array_merge($state, $changes);
            $state['command_id'] = ((int) $state['command_id']) + 1;
            $state['updated_at'] = gmdate('c');
            $this->write($state);
            return $state;
        });
    }

    public function reset(): array
    {
        return $this->locked(function (): array {
            return $this->resetUnlocked();
        });
    }

    public function resetUnlocked(): array
    {
        $state = $this->defaultState();
        $this->write($state);
        return $state;
    }

    public function locked(callable $work): mixed
    {
        $lockPath = auxiot_config()['paths']['lock'];
        $lock = fopen($lockPath, 'c');
        if ($lock === false) {
            throw new RuntimeException('Unable to open state lock.');
        }

        if (!flock($lock, LOCK_EX | LOCK_NB)) {
            fclose($lock);
            throw new RuntimeException('Another state update is already running.');
        }

        try {
            return $work();
        } finally {
            flock($lock, LOCK_UN);
            fclose($lock);
        }
    }

    private function write(array $state): void
    {
        $path = auxiot_config()['paths']['state'];
        $tmp = $path . '.tmp';
        $json = json_encode($state, JSON_PRETTY_PRINT | JSON_UNESCAPED_SLASHES);
        if ($json === false || file_put_contents($tmp, $json, LOCK_EX) === false) {
            throw new RuntimeException('Unable to write state.');
        }
        rename($tmp, $path);
    }
}
