<?php

declare(strict_types=1);

final class ResetService
{
    public function playback(): array
    {
        return (new StateStore())->command([
            'action' => 'stop',
            'url' => '',
            'title' => '',
            'is_playing' => false,
            'playlist_id' => null,
            'track_id' => null,
            'playback_index' => 0,
        ]);
    }

    public function server(): array
    {
        return (new StateStore())->reset();
    }

    public function library(): void
    {
        $pdo = Database::pdo();
        $state = new StateStore();
        $state->locked(function () use ($pdo): void {
            $pdo->beginTransaction();
            try {
                (new TrackRepository())->deleteAllUploads();
                $pdo->exec('DELETE FROM playlist_tracks');
                $pdo->exec('DELETE FROM playlists');
                $pdo->exec('DELETE FROM tracks');
                $pdo->commit();
            } catch (Throwable $e) {
                $pdo->rollBack();
                throw $e;
            }
        });
    }

    public function full(): array
    {
        $pdo = Database::pdo();
        $state = new StateStore();
        return $state->locked(function () use ($pdo, $state): array {
            $pdo->beginTransaction();
            try {
                (new TrackRepository())->deleteAllUploads();
                $pdo->exec('DELETE FROM playlist_tracks');
                $pdo->exec('DELETE FROM playlists');
                $pdo->exec('DELETE FROM tracks');
                $pdo->exec('DELETE FROM device_status');
                $pdo->commit();
                return $state->resetUnlocked();
            } catch (Throwable $e) {
                $pdo->rollBack();
                throw $e;
            }
        });
    }
}
