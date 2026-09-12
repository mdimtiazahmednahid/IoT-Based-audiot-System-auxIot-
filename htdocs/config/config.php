<?php

declare(strict_types=1);

$local = __DIR__ . '/local.php';
$overrides = is_file($local) ? require $local : [];

$config = [
    'db' => [
        'dsn' => getenv('AUXIOT_DB_DSN') ?: 'mysql:host=127.0.0.1;dbname=auxiot;charset=utf8mb4',
        'user' => getenv('AUXIOT_DB_USER') ?: 'root',
        'pass' => getenv('AUXIOT_DB_PASS') ?: '',
    ],
    'paths' => [
        'root' => dirname(__DIR__),
        'storage' => dirname(__DIR__) . DIRECTORY_SEPARATOR . 'storage',
        'uploads' => dirname(__DIR__) . DIRECTORY_SEPARATOR . 'uploads',
        'state' => dirname(__DIR__) . DIRECTORY_SEPARATOR . 'storage' . DIRECTORY_SEPARATOR . 'state.json',
        'lock' => dirname(__DIR__) . DIRECTORY_SEPARATOR . 'storage' . DIRECTORY_SEPARATOR . 'state.lock',
    ],
    'uploads' => [
        'max_bytes' => 25 * 1024 * 1024,
        'public_prefix' => getenv('AUXIOT_UPLOAD_PUBLIC_PREFIX') ?: 'https://aiotamp.rf.gd/uploads/',
    ],
];

return array_replace_recursive($config, $overrides);
