<?php

declare(strict_types=1);

final class AuthService
{
    public function __construct()
    {
        Database::pdo()->exec(
            'CREATE TABLE IF NOT EXISTS users (
                id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
                email VARCHAR(255) NOT NULL UNIQUE,
                passcode_hash VARCHAR(255) NOT NULL,
                created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
            ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci'
        );
    }

    public function currentUser(): ?array
    {
        if (empty($_SESSION['user_id'])) {
            return null;
        }

        $stmt = Database::pdo()->prepare('SELECT id, email, created_at FROM users WHERE id = ?');
        $stmt->execute([(int) $_SESSION['user_id']]);
        $user = $stmt->fetch();

        if (!$user) {
            $this->logout();
            return null;
        }

        return $user;
    }

    public function signup(string $email, string $passcode): array
    {
        $email = $this->email($email);
        $passcode = $this->passcode($passcode);

        $stmt = Database::pdo()->prepare('SELECT id FROM users WHERE email = ?');
        $stmt->execute([$email]);
        if ($stmt->fetch()) {
            throw new InvalidArgumentException('This email is already registered.');
        }

        $hash = password_hash($passcode, PASSWORD_DEFAULT);
        $insert = Database::pdo()->prepare('INSERT INTO users (email, passcode_hash) VALUES (?, ?)');
        $insert->execute([$email, $hash]);

        $_SESSION['user_id'] = (int) Database::pdo()->lastInsertId();
        session_regenerate_id(true);

        return $this->currentUser() ?? ['email' => $email];
    }

    public function signin(string $email, string $passcode): array
    {
        $email = $this->email($email);
        $passcode = $this->passcode($passcode);

        $stmt = Database::pdo()->prepare('SELECT * FROM users WHERE email = ?');
        $stmt->execute([$email]);
        $user = $stmt->fetch();
        if (!$user || !password_verify($passcode, (string) $user['passcode_hash'])) {
            throw new InvalidArgumentException('Email or passcode is incorrect.');
        }

        $_SESSION['user_id'] = (int) $user['id'];
        session_regenerate_id(true);

        return [
            'id' => (int) $user['id'],
            'email' => (string) $user['email'],
            'created_at' => (string) $user['created_at'],
        ];
    }

    public function logout(): void
    {
        $_SESSION = [];
        if (session_status() === PHP_SESSION_ACTIVE) {
            session_destroy();
        }
    }

    private function email(string $email): string
    {
        $email = strtolower(trim($email));
        if (!filter_var($email, FILTER_VALIDATE_EMAIL) || strlen($email) > 255) {
            throw new InvalidArgumentException('Enter a valid email address.');
        }
        return $email;
    }

    private function passcode(string $passcode): string
    {
        $passcode = trim($passcode);
        if (!preg_match('/^\d{4}$/', $passcode)) {
            throw new InvalidArgumentException('Passcode must be exactly 4 digits.');
        }
        return $passcode;
    }
}
