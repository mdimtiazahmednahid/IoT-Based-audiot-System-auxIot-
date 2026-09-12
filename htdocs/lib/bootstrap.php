<?php

declare(strict_types=1);

spl_autoload_register(static function (string $class): void {
    $file = __DIR__ . DIRECTORY_SEPARATOR . $class . '.php';
    if (is_file($file)) {
        require_once $file;
    }
});

$config = require __DIR__ . '/../config/config.php';

foreach (['storage', 'uploads'] as $key) {
    if (!is_dir($config['paths'][$key])) {
        mkdir($config['paths'][$key], 0775, true);
    }
}

function auxiot_config(): array
{
    static $loaded;
    if ($loaded === null) {
        $loaded = require __DIR__ . '/../config/config.php';
    }
    return $loaded;
}
