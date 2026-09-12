<?php

declare(strict_types=1);

final class PlaylistRepository
{
    public function all(): array
    {
        $playlists = Database::pdo()->query('SELECT * FROM playlists ORDER BY created_at DESC, id DESC')->fetchAll();
        $stmt = Database::pdo()->prepare(
            'SELECT pt.id AS playlist_track_id, pt.playlist_id, pt.sort_order, t.* FROM playlist_tracks pt JOIN tracks t ON t.id = pt.track_id WHERE pt.playlist_id = ? ORDER BY pt.sort_order, pt.id'
        );
        foreach ($playlists as &$playlist) {
            $stmt->execute([(int) $playlist['id']]);
            $playlist['tracks'] = $stmt->fetchAll();
        }
        return $playlists;
    }

    public function create(string $name): array
    {
        $stmt = Database::pdo()->prepare('INSERT INTO playlists (name) VALUES (?)');
        $stmt->execute([$name]);
        return ['id' => (int) Database::pdo()->lastInsertId(), 'name' => $name, 'tracks' => []];
    }

    public function addTrack(int $playlistId, int $trackId): void
    {
        $stmt = Database::pdo()->prepare('SELECT COALESCE(MAX(sort_order), 0) + 1 FROM playlist_tracks WHERE playlist_id = ?');
        $stmt->execute([$playlistId]);
        $sort = (int) $stmt->fetchColumn();
        $insert = Database::pdo()->prepare('INSERT INTO playlist_tracks (playlist_id, track_id, sort_order) VALUES (?, ?, ?)');
        $insert->execute([$playlistId, $trackId, $sort]);
    }

    public function removeTrack(int $playlistTrackId): void
    {
        $stmt = Database::pdo()->prepare('DELETE FROM playlist_tracks WHERE id = ?');
        $stmt->execute([$playlistTrackId]);
    }

    public function firstTrack(int $playlistId): ?array
    {
        $stmt = Database::pdo()->prepare(
            'SELECT t.*, pt.sort_order, pt.id AS playlist_track_id FROM playlist_tracks pt JOIN tracks t ON t.id = pt.track_id WHERE pt.playlist_id = ? ORDER BY pt.sort_order, pt.id LIMIT 1'
        );
        $stmt->execute([$playlistId]);
        $track = $stmt->fetch();
        return $track ?: null;
    }

    public function tracks(int $playlistId): array
    {
        $stmt = Database::pdo()->prepare(
            'SELECT t.*, pt.sort_order, pt.id AS playlist_track_id FROM playlist_tracks pt JOIN tracks t ON t.id = pt.track_id WHERE pt.playlist_id = ? ORDER BY pt.sort_order, pt.id'
        );
        $stmt->execute([$playlistId]);
        return $stmt->fetchAll();
    }

    public function adjacentTrack(int $playlistId, ?int $trackId, string $direction): ?array
    {
        $tracks = $this->tracks($playlistId);
        if (!$tracks) {
            return null;
        }

        $currentIndex = 0;
        if ($trackId !== null) {
            foreach ($tracks as $index => $track) {
                if ((int) $track['id'] === $trackId) {
                    $currentIndex = $index;
                    break;
                }
            }
        }

        $offset = $direction === 'previous' ? -1 : 1;
        $nextIndex = ($currentIndex + $offset + count($tracks)) % count($tracks);
        return $tracks[$nextIndex] + ['playback_index' => $nextIndex];
    }
}
