<?php

declare(strict_types=1);

final class Validator
{
    public static function title(string $title): string
    {
        $clean = trim(preg_replace('/\s+/', ' ', $title));
        if ($clean === '' || mb_strlen($clean) > 255) {
            throw new InvalidArgumentException('Title must be 1-255 characters.');
        }
        return $clean;
    }

    public static function playlistName(string $name): string
    {
        return self::title($name);
    }

    public static function id(mixed $value, string $name = 'id'): int
    {
        $id = filter_var($value, FILTER_VALIDATE_INT, ['options' => ['min_range' => 1]]);
        if ($id === false) {
            throw new InvalidArgumentException("Invalid {$name}.");
        }
        return (int) $id;
    }

    public static function volume(mixed $value): int
    {
        $volume = filter_var($value, FILTER_VALIDATE_INT, ['options' => ['min_range' => 0, 'max_range' => 21]]);
        if ($volume === false) {
            throw new InvalidArgumentException('Volume must be between 0 and 21.');
        }
        return (int) $volume;
    }

    public static function mp3Url(string $url): string
    {
        $url = trim($url);
        if (!filter_var($url, FILTER_VALIDATE_URL)) {
            throw new InvalidArgumentException('URL is invalid.');
        }
        $scheme = strtolower((string) parse_url($url, PHP_URL_SCHEME));
        if (!in_array($scheme, ['http', 'https'], true)) {
            throw new InvalidArgumentException('Only HTTP and HTTPS stream URLs are accepted.');
        }
        $host = parse_url($url, PHP_URL_HOST) ?: '';
        if (preg_match('/(^|\.)youtube\.com$|(^|\.)youtu\.be$|(^|\.)spotify\.com$|(^|\.)soundcloud\.com$|(^|\.)facebook\.com$|(^|\.)instagram\.com$/i', $host)) {
            throw new InvalidArgumentException('Only direct audio stream URLs are accepted.');
        }

        $path = parse_url($url, PHP_URL_PATH) ?: '';
        if (preg_match('#/(watch|audio/play|video|playlist|album|track|episode)(/|$)#i', $path)) {
            throw new InvalidArgumentException('URL must be a direct MP3 stream, not a browser player page.');
        }
        if (preg_match('/\.(html?|php|aspx?|jsp|pdf|zip|rar|7z|jpg|jpeg|png|gif|webp|svg)$/i', $path)) {
            throw new InvalidArgumentException('URL must point to an audio stream, not a web page or file download.');
        }

        return $url;
    }
}
