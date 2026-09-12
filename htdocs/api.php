<?php

declare(strict_types=1);

require __DIR__ . '/lib/bootstrap.php';

Response::noStore();

try {
    $action = $_GET['action'] ?? $_POST['action'] ?? '';
    $method = $_SERVER['REQUEST_METHOD'] ?? 'GET';

    if ($method === 'GET') {
        match ($action) {
            'state' => Response::json(['ok' => true, 'state' => enriched_state()]),
            'library' => Response::json(['ok' => true, 'tracks' => (new TrackRepository())->all()]),
            'playlists' => Response::json(['ok' => true, 'playlists' => (new PlaylistRepository())->all()]),
            'device_status' => Response::json(['ok' => true, 'devices' => (new DeviceRepository())->all()]),
            'stream' => stream_track(),
            default => Response::json(['ok' => false, 'error' => 'Unknown endpoint.'], 404),
        };
    }

    $payload = input_payload();

    match ($action) {
        'control' => control($payload),
        'upload' => upload_track(),
        'add_link' => add_link($payload),
        'rename_track' => rename_track($payload),
        'delete_track' => delete_track($payload),
        'playlist_create' => playlist_create($payload),
        'playlist_add' => playlist_add($payload),
        'playlist_remove' => playlist_remove($payload),
        'play_playlist' => play_playlist($payload),
        'playlist_nav' => playlist_nav($payload),
        'device_status' => device_status($payload),
        'reset' => reset_action($payload),
        default => Response::json(['ok' => false, 'error' => 'Unknown endpoint.'], 404),
    };
} catch (InvalidArgumentException $e) {
    Response::json(['ok' => false, 'error' => $e->getMessage()], 422);
} catch (RuntimeException $e) {
    Response::json(['ok' => false, 'error' => $e->getMessage()], 409);
} catch (Throwable $e) {
    Response::json(['ok' => false, 'error' => 'Server error.', 'detail' => $e->getMessage()], 500);
}

function input_payload(): array
{
    $type = $_SERVER['CONTENT_TYPE'] ?? '';
    if (str_contains($type, 'application/json')) {
        $raw = file_get_contents('php://input') ?: '{}';
        $decoded = json_decode($raw, true);
        return is_array($decoded) ? $decoded : [];
    }
    return $_POST;
}

function enriched_state(): array
{
    $state = (new StateStore())->read();
    if (!empty($state['track_id'])) {
        $track = (new TrackRepository())->find((int) $state['track_id']);
        if ($track) {
            $state['duration_seconds'] = $track['duration_seconds'] ?? null;
            if (empty($state['source_type'])) {
                $state['source_type'] = $track['source_type'] ?? null;
            }
        }
    }
    return $state;
}

function control(array $payload): never
{
    $control = (string) ($payload['control'] ?? '');
    $state = new StateStore();

    if ($control === 'play') {
        $trackId = Validator::id($payload['track_id'] ?? null, 'track_id');
        $track = (new TrackRepository())->find($trackId);
        if (!$track) {
            throw new InvalidArgumentException('Track not found.');
        }
        $next = $state->command([
            'action' => 'play',
            'url' => playback_url($track),
            'title' => $track['title'],
            'track_id' => (int) $track['id'],
            'duration_seconds' => $track['duration_seconds'] ?? null,
            'playlist_id' => $payload['playlist_id'] ?? null,
            'is_playing' => true,
        ]);
        Response::json(['ok' => true, 'state' => $next]);
    }

    if (in_array($control, ['stop', 'pause'], true)) {
        $next = $state->command([
            'action' => 'stop',
            'url' => '',
            'title' => '',
            'is_playing' => false,
            'track_id' => null,
            'duration_seconds' => null,
        ]);
        Response::json(['ok' => true, 'state' => $next]);
    }

    if ($control === 'volume') {
        $volume = Validator::volume($payload['volume'] ?? null);
        $next = $state->command(['action' => 'volume', 'volume' => $volume]);
        Response::json(['ok' => true, 'state' => $next]);
    }

    if ($control === 'auto_play') {
        $enabled = filter_var($payload['auto_play'] ?? false, FILTER_VALIDATE_BOOL);
        $next = $state->command(['action' => 'auto_play', 'auto_play' => $enabled]);
        Response::json(['ok' => true, 'state' => $next]);
    }

    Response::json(['ok' => false, 'error' => 'Invalid control.'], 422);
}

function upload_track(): never
{
    if (!isset($_FILES['file']) || !is_uploaded_file($_FILES['file']['tmp_name'])) {
        throw new InvalidArgumentException('MP3 file is required.');
    }

    $file = $_FILES['file'];
    $max = auxiot_config()['uploads']['max_bytes'];
    if (($file['size'] ?? 0) < 1 || ($file['size'] ?? 0) > $max) {
        throw new InvalidArgumentException('File size is invalid.');
    }

    $original = (string) ($file['name'] ?? '');
    if (!preg_match('/\.mp3$/i', $original)) {
        throw new InvalidArgumentException('Only MP3 uploads are accepted.');
    }

    $mime = (new finfo(FILEINFO_MIME_TYPE))->file($file['tmp_name']) ?: '';
    if (!in_array($mime, ['audio/mpeg', 'audio/mp3', 'application/octet-stream'], true)) {
        throw new InvalidArgumentException('Uploaded file is not a valid MP3.');
    }

    $title = Validator::title($_POST['title'] ?? pathinfo($original, PATHINFO_FILENAME));
    $safe = bin2hex(random_bytes(12)) . '.mp3';
    $target = auxiot_config()['paths']['uploads'] . DIRECTORY_SEPARATOR . $safe;
    if (!move_uploaded_file($file['tmp_name'], $target)) {
        throw new RuntimeException('Unable to save upload.');
    }

    $url = auxiot_config()['uploads']['public_prefix'] . $safe;
    $track = (new TrackRepository())->createUpload($title, $url, $target, 'audio/mpeg');
    Response::json(['ok' => true, 'track' => $track]);
}

function add_link(array $payload): never
{
    $title = Validator::title((string) ($payload['title'] ?? 'Untitled stream'));
    $url = Validator::mp3Url((string) ($payload['url'] ?? ''));
    $track = (new TrackRepository())->createLink($title, $url);
    Response::json(['ok' => true, 'track' => $track]);
}

function delete_track(array $payload): never
{
    (new TrackRepository())->delete(Validator::id($payload['track_id'] ?? null, 'track_id'));
    Response::json(['ok' => true]);
}

function rename_track(array $payload): never
{
    $track = (new TrackRepository())->rename(
        Validator::id($payload['track_id'] ?? null, 'track_id'),
        Validator::title((string) ($payload['title'] ?? ''))
    );
    if (!$track) {
        throw new InvalidArgumentException('Track not found.');
    }
    Response::json(['ok' => true, 'track' => $track]);
}

function playlist_create(array $payload): never
{
    $playlist = (new PlaylistRepository())->create(Validator::playlistName((string) ($payload['name'] ?? '')));
    Response::json(['ok' => true, 'playlist' => $playlist]);
}

function playlist_add(array $payload): never
{
    (new PlaylistRepository())->addTrack(
        Validator::id($payload['playlist_id'] ?? null, 'playlist_id'),
        Validator::id($payload['track_id'] ?? null, 'track_id')
    );
    Response::json(['ok' => true]);
}

function playlist_remove(array $payload): never
{
    (new PlaylistRepository())->removeTrack(Validator::id($payload['playlist_track_id'] ?? null, 'playlist_track_id'));
    Response::json(['ok' => true]);
}

function play_playlist(array $payload): never
{
    $playlistId = Validator::id($payload['playlist_id'] ?? null, 'playlist_id');
    $track = (new PlaylistRepository())->firstTrack($playlistId);
    if (!$track) {
        throw new InvalidArgumentException('Playlist has no tracks.');
    }
    $next = (new StateStore())->command([
        'action' => 'play',
        'url' => playback_url($track),
        'title' => $track['title'],
        'track_id' => (int) $track['id'],
        'duration_seconds' => $track['duration_seconds'] ?? null,
        'playlist_id' => $playlistId,
        'playback_index' => 0,
        'is_playing' => true,
    ]);
    Response::json(['ok' => true, 'state' => $next]);
}

function playlist_nav(array $payload): never
{
    $direction = (string) ($payload['direction'] ?? 'next');
    if (!in_array($direction, ['next', 'previous'], true)) {
        throw new InvalidArgumentException('Invalid playlist direction.');
    }

    $stateStore = new StateStore();
    $current = $stateStore->read();
    $playlistId = isset($current['playlist_id']) ? (int) $current['playlist_id'] : 0;
    if ($playlistId < 1) {
        throw new InvalidArgumentException('Start a playlist first.');
    }

    $trackId = isset($current['track_id']) ? (int) $current['track_id'] : null;
    $track = (new PlaylistRepository())->adjacentTrack($playlistId, $trackId, $direction);
    if (!$track) {
        throw new InvalidArgumentException('Playlist has no tracks.');
    }

    $next = $stateStore->command([
        'action' => 'play',
        'url' => playback_url($track),
        'title' => $track['title'],
        'track_id' => (int) $track['id'],
        'duration_seconds' => $track['duration_seconds'] ?? null,
        'playlist_id' => $playlistId,
        'playback_index' => (int) ($track['playback_index'] ?? 0),
        'is_playing' => true,
    ]);
    Response::json(['ok' => true, 'state' => $next]);
}

function device_status(array $payload): never
{
    (new DeviceRepository())->upsert($payload);
    Response::json(['ok' => true]);
}

function reset_action(array $payload): never
{
    $type = (string) ($payload['type'] ?? '');
    $confirm = (string) ($payload['confirm'] ?? '');
    $service = new ResetService();
    $state = new StateStore();

    if (in_array($type, ['factory_device', 'library', 'full'], true) && $confirm !== 'RESET') {
        throw new InvalidArgumentException('Type RESET to confirm this destructive action.');
    }

    match ($type) {
        'playback' => Response::json(['ok' => true, 'state' => $service->playback()]),
        'soft_device' => Response::json(['ok' => true, 'state' => $state->command(['action' => 'soft_reset', 'is_playing' => false])]),
        'wifi_setup' => Response::json(['ok' => true, 'state' => $state->command(['action' => 'wifi_setup', 'is_playing' => false])]),
        'factory_device' => Response::json(['ok' => true, 'state' => $state->command(['action' => 'factory_reset', 'is_playing' => false])]),
        'server' => Response::json(['ok' => true, 'state' => $service->server()]),
        'library' => do_library_reset($service),
        'full' => Response::json(['ok' => true, 'state' => $service->full()]),
        default => Response::json(['ok' => false, 'error' => 'Invalid reset type.'], 422),
    };
}

function do_library_reset(ResetService $service): never
{
    $service->library();
    Response::json(['ok' => true]);
}

function playback_url(array $track): string
{
    $url = (string) ($track['file_url'] ?? '');
    if (($track['source_type'] ?? '') !== 'upload') {
        return $url;
    }
    if (isset($track['id'])) {
        return 'https://aiotamp.rf.gd/api.php?action=stream&track_id=' . (int) $track['id'];
    }
    if (str_starts_with($url, 'http://aiotamp.rf.gd/uploads/')) {
        return 'https://' . substr($url, 7);
    }
    if (str_starts_with($url, '/uploads/')) {
        return 'https://aiotamp.rf.gd' . $url;
    }
    return $url;
}

function stream_track(): never
{
    $trackId = Validator::id($_GET['track_id'] ?? null, 'track_id');
    $track = (new TrackRepository())->find($trackId);
    if (!$track || ($track['source_type'] ?? '') !== 'upload' || empty($track['file_path'])) {
        http_response_code(404);
        exit;
    }

    $uploads = realpath(auxiot_config()['paths']['uploads']);
    $path = realpath((string) $track['file_path']);
    if ($uploads === false || $path === false || !str_starts_with($path, $uploads . DIRECTORY_SEPARATOR) || !is_file($path)) {
        http_response_code(404);
        exit;
    }

    $size = filesize($path);
    $start = 0;
    $end = $size - 1;
    $status = 200;

    $range = $_SERVER['HTTP_RANGE'] ?? '';
    if (preg_match('/bytes=(\d*)-(\d*)/', $range, $match)) {
        if ($match[1] !== '') {
            $start = max(0, (int) $match[1]);
        }
        if ($match[2] !== '') {
            $end = min($end, (int) $match[2]);
        }
        if ($start <= $end) {
            $status = 206;
        }
    }

    http_response_code($status);
    header('Content-Type: audio/mpeg');
    header('Accept-Ranges: bytes');
    header('Cache-Control: no-store, no-cache, must-revalidate, max-age=0');
    if ($status === 206) {
        header("Content-Range: bytes {$start}-{$end}/{$size}");
    }
    header('Content-Length: ' . (($end - $start) + 1));

    $handle = fopen($path, 'rb');
    if ($handle === false) {
        http_response_code(500);
        exit;
    }
    fseek($handle, $start);
    $remaining = ($end - $start) + 1;
    while ($remaining > 0 && !feof($handle)) {
        $chunk = fread($handle, min(8192, $remaining));
        if ($chunk === false) {
            break;
        }
        echo $chunk;
        $remaining -= strlen($chunk);
        flush();
    }
    fclose($handle);
    exit;
}
