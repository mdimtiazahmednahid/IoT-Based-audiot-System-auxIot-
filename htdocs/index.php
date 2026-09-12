<?php

declare(strict_types=1);

header('Cache-Control: no-store, no-cache, must-revalidate, max-age=0');
?>
<!doctype html>
<html lang="en">
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <meta name="theme-color" content="#07080d">
    <title>AuxIoT</title>
    <script>
        (() => {
            const saved = localStorage.getItem('auxiot-theme');
            const light = saved ? saved === 'light' : matchMedia('(prefers-color-scheme: light)').matches;
            document.documentElement.dataset.theme = light ? 'light' : 'dark';
            document.querySelector('meta[name="theme-color"]').setAttribute('content', light ? '#f5f7fb' : '#07080d');
        })();
    </script>
    <link rel="manifest" href="manifest.json">
    <link rel="apple-touch-icon" href="icons/icon-192.png">
    <link rel="stylesheet" href="assets/app.css?v=33">
    <script src="assets/app.js?v=33" defer></script>
</head>
<body>
    <div class="ambient"></div>
    <button class="device-status-pill status-offline-bg" id="deviceStatusButton" type="button" title="Open device monitor">
        <span class="device-glyph" aria-hidden="true">
            <svg viewBox="0 0 24 24"><path d="M6 8.5h12v7H6z"/><path d="M9 5.5h6M9 18.5h6M3.5 10.5v3M20.5 10.5v3"/></svg>
        </span>
        <span class="status-dot"></span>
        <span id="deviceStatusText">Device</span>
        <small id="deviceStatusMeta">Waiting for telemetry</small>
    </button>

    <div class="app-shell">
        <header class="app-header">
            <div class="brand">
                <span class="brand-mark" aria-hidden="true">
                    <svg viewBox="0 0 24 24"><path d="M9 17.5a3 3 0 1 1-2-2.83V6.8L18 4v10.7a3 3 0 1 1-2-2.83V7.2l-7 1.76z"/></svg>
                </span>
                <div>
                    <strong>AuxIoT</strong>
                </div>
            </div>
            <div class="header-actions">
                <button class="icon-only" id="themeToggle" type="button" title="Switch theme" aria-label="Switch theme">
                    <svg class="theme-sun" viewBox="0 0 24 24" aria-hidden="true"><path d="M12 4V2m0 20v-2M4.93 4.93 3.51 3.51m16.98 16.98-1.42-1.42M4 12H2m20 0h-2M4.93 19.07l-1.42 1.42M20.49 3.51l-1.42 1.42"/><circle cx="12" cy="12" r="4"/></svg>
                    <svg class="theme-moon" viewBox="0 0 24 24" aria-hidden="true"><path d="M20 14.2A7.6 7.6 0 0 1 9.8 4a8.4 8.4 0 1 0 10.2 10.2Z"/></svg>
                </button>
                <button class="icon-only" id="settingsButton" type="button" title="Settings" aria-label="Settings">
                    <svg viewBox="0 0 24 24" aria-hidden="true"><path d="M12 15.5a3.5 3.5 0 1 0 0-7 3.5 3.5 0 0 0 0 7Z"/><path d="M19.4 15a1.8 1.8 0 0 0 .36 1.98l.04.04a2 2 0 1 1-2.83 2.83l-.04-.04A1.8 1.8 0 0 0 15 19.4a1.8 1.8 0 0 0-1 .6 1.8 1.8 0 0 0-.4 1.1V21a2 2 0 1 1-4 0v-.06A1.8 1.8 0 0 0 8.4 19.4a1.8 1.8 0 0 0-1.98.36l-.04.04a2 2 0 1 1-2.83-2.83l.04-.04A1.8 1.8 0 0 0 4.6 15a1.8 1.8 0 0 0-.6-1 1.8 1.8 0 0 0-1.1-.4H3a2 2 0 1 1 0-4h.06A1.8 1.8 0 0 0 4.6 8.4a1.8 1.8 0 0 0-.36-1.98l-.04-.04a2 2 0 1 1 2.83-2.83l.04.04A1.8 1.8 0 0 0 9 4.6a1.8 1.8 0 0 0 1-.6 1.8 1.8 0 0 0 .4-1.1V3a2 2 0 1 1 4 0v.06A1.8 1.8 0 0 0 15.6 4.6a1.8 1.8 0 0 0 1.98-.36l.04-.04a2 2 0 1 1 2.83 2.83l-.04.04A1.8 1.8 0 0 0 19.4 9c.27.32.6.55 1 .6.28.04.57.04.85.04H21a2 2 0 1 1 0 4h-.06a1.8 1.8 0 0 0-1.54 1.36Z"/></svg>
                </button>
            </div>
        </header>

        <main class="main-panel">
            <section class="now-playing" id="nowPlaying">
                <div class="sea-wave" id="seaWave" aria-hidden="true">
                    <span></span><span></span><span></span><span></span>
                </div>
                <div class="hero-topline">
                    <p>Now Playing</p>
                    <span id="currentMeta">Ready</span>
                </div>
                <div class="turntable" id="turntable" role="group" aria-label="Playlist navigation disc">
                    <button class="disc-nav disc-prev" type="button" data-playlist-nav="previous" aria-label="Previous playlist track">
                        <svg viewBox="0 0 24 24" aria-hidden="true"><path d="M11 6 5 12l6 6V6Zm8 0-6 6 6 6V6Z"/></svg>
                    </button>
                    <div class="disc-face" aria-hidden="true"></div>
                    <button class="playback-toggle disc-playback-toggle" id="heroPlaybackToggle" type="button" aria-label="Play or pause">
                        <svg class="icon-play" viewBox="0 0 24 24" aria-hidden="true"><path d="M8 5v14l11-7z"/></svg>
                        <svg class="icon-pause" viewBox="0 0 24 24" aria-hidden="true"><path d="M8 5h3v14H8zM13 5h3v14h-3z"/></svg>
                    </button>
                    <button class="disc-nav disc-next" type="button" data-playlist-nav="next" aria-label="Next playlist track">
                        <svg viewBox="0 0 24 24" aria-hidden="true"><path d="m5 6 6 6-6 6V6Zm8 0 6 6-6 6V6Z"/></svg>
                    </button>
                    <button class="disc-picker-toggle" id="discPickerToggle" type="button" aria-label="Select playlist" title="Select playlist">
                        <svg viewBox="0 0 24 24" aria-hidden="true"><path d="M8 6h13M8 12h13M8 18h13M3 6h.01M3 12h.01M3 18h.01"/></svg>
                    </button>
                    <div class="disc-playlist-picker" id="discPlaylistPicker" aria-live="polite"></div>
                </div>
                <h1 id="currentTitle">Nothing playing</h1>
                <div class="playback-insight" id="playbackInsight" aria-live="polite">
                    <span class="duration-chip" id="heroDuration">00:00</span>
                    <span class="equalizer" id="heroEqualizer" aria-hidden="true">
                        <i></i><i></i><i></i><i></i><i></i>
                    </span>
                </div>
                <p class="hero-hint" id="heroHint">Add a stream, upload an MP3, or choose a playlist from the disc.</p>
                <div class="quick-actions" aria-label="Quick actions">
                    <a class="icon-action" href="#library" title="Open library">
                        <svg viewBox="0 0 24 24" aria-hidden="true"><path d="M4 6h16M4 12h16M4 18h10"/></svg>
                        <span class="sr-only">Library</span>
                    </a>
                    <a class="icon-action" href="#upload" title="Upload MP3">
                        <svg viewBox="0 0 24 24" aria-hidden="true"><path d="M12 16V4m0 0 4 4m-4-4L8 8M5 16v2a2 2 0 0 0 2 2h10a2 2 0 0 0 2-2v-2"/></svg>
                        <span class="sr-only">Upload</span>
                    </a>
                    <a class="icon-action" href="#add-url" title="Add stream URL">
                        <svg viewBox="0 0 24 24" aria-hidden="true"><path d="M10 13a5 5 0 0 0 7.07 0l2.12-2.12a5 5 0 0 0-7.07-7.07L11 4.93m3 6.14a5 5 0 0 0-7.07 0L4.81 13.2a5 5 0 0 0 7.07 7.07L13 19.07"/></svg>
                        <span class="sr-only">Add Link</span>
                    </a>
                    <button class="icon-action" id="deviceQuickButton" type="button" title="Open device monitor">
                        <svg viewBox="0 0 24 24" aria-hidden="true"><path d="M6 8.5h12v7H6z"/><path d="M9 5.5h6M9 18.5h6M3.5 10.5v3M20.5 10.5v3"/></svg>
                        <span class="sr-only">Device</span>
                    </button>
                    <button class="icon-action" id="settingsQuickButton" type="button" title="Open settings">
                        <svg viewBox="0 0 24 24" aria-hidden="true"><path d="M12 15.5a3.5 3.5 0 1 0 0-7 3.5 3.5 0 0 0 0 7Z"/><path d="M19.4 15a1.8 1.8 0 0 0 .36 1.98l.04.04a2 2 0 1 1-2.83 2.83l-.04-.04A1.8 1.8 0 0 0 15 19.4a1.8 1.8 0 0 0-1 .6 1.8 1.8 0 0 0-.4 1.1V21a2 2 0 1 1-4 0v-.06A1.8 1.8 0 0 0 8.4 19.4a1.8 1.8 0 0 0-1.98.36l-.04.04a2 2 0 1 1-2.83-2.83l.04-.04A1.8 1.8 0 0 0 4.6 15a1.8 1.8 0 0 0-.6-1 1.8 1.8 0 0 0-1.1-.4H3a2 2 0 1 1 0-4h.06A1.8 1.8 0 0 0 4.6 8.4a1.8 1.8 0 0 0-.36-1.98l-.04-.04a2 2 0 1 1 2.83-2.83l.04.04A1.8 1.8 0 0 0 9 4.6a1.8 1.8 0 0 0 1-.6 1.8 1.8 0 0 0 .4-1.1V3a2 2 0 1 1 4 0v.06A1.8 1.8 0 0 0 15.6 4.6a1.8 1.8 0 0 0 1.98-.36l.04-.04a2 2 0 1 1 2.83 2.83l-.04.04A1.8 1.8 0 0 0 19.4 9c.27.32.6.55 1 .6.28.04.57.04.85.04H21a2 2 0 1 1 0 4h-.06a1.8 1.8 0 0 0-1.54 1.36Z"/></svg>
                        <span class="sr-only">Settings</span>
                    </button>
                </div>
            </section>

            <nav class="feature-dock" aria-label="Primary sections">
                <a href="#library">
                    <svg viewBox="0 0 24 24" aria-hidden="true"><path d="M4 6h16M4 12h16M4 18h10"/></svg>
                    <strong>Library</strong>
                </a>
                <a href="#playlists">
                    <svg viewBox="0 0 24 24" aria-hidden="true"><path d="M8 6h13M8 12h13M8 18h13M3 6h.01M3 12h.01M3 18h.01"/></svg>
                    <strong>Playlists</strong>
                </a>
                <a href="#upload">
                    <svg viewBox="0 0 24 24" aria-hidden="true"><path d="M12 16V4m0 0 4 4m-4-4L8 8M5 16v2a2 2 0 0 0 2 2h10a2 2 0 0 0 2-2v-2"/></svg>
                    <strong>Upload</strong>
                </a>
                <a href="#add-url">
                    <svg viewBox="0 0 24 24" aria-hidden="true"><path d="M10 13a5 5 0 0 0 7.07 0l2.12-2.12a5 5 0 0 0-7.07-7.07L11 4.93m3 6.14a5 5 0 0 0-7.07 0L4.81 13.2a5 5 0 0 0 7.07 7.07L13 19.07"/></svg>
                    <strong>URL</strong>
                </a>
                <button id="settingsDockButton" type="button">
                    <svg viewBox="0 0 24 24" aria-hidden="true"><path d="M12 15.5a3.5 3.5 0 1 0 0-7 3.5 3.5 0 0 0 0 7Z"/><path d="M19.4 15a1.8 1.8 0 0 0 .36 1.98l.04.04a2 2 0 1 1-2.83 2.83l-.04-.04A1.8 1.8 0 0 0 15 19.4a1.8 1.8 0 0 0-1 .6 1.8 1.8 0 0 0-.4 1.1V21a2 2 0 1 1-4 0v-.06A1.8 1.8 0 0 0 8.4 19.4a1.8 1.8 0 0 0-1.98.36l-.04.04a2 2 0 1 1-2.83-2.83l.04-.04A1.8 1.8 0 0 0 4.6 15a1.8 1.8 0 0 0-.6-1 1.8 1.8 0 0 0-1.1-.4H3a2 2 0 1 1 0-4h.06A1.8 1.8 0 0 0 4.6 8.4a1.8 1.8 0 0 0-.36-1.98l-.04-.04a2 2 0 1 1 2.83-2.83l.04.04A1.8 1.8 0 0 0 9 4.6a1.8 1.8 0 0 0 1-.6 1.8 1.8 0 0 0 .4-1.1V3a2 2 0 1 1 4 0v.06A1.8 1.8 0 0 0 15.6 4.6a1.8 1.8 0 0 0 1.98-.36l.04-.04a2 2 0 1 1 2.83 2.83l-.04.04A1.8 1.8 0 0 0 19.4 9c.27.32.6.55 1 .6.28.04.57.04.85.04H21a2 2 0 1 1 0 4h-.06a1.8 1.8 0 0 0-1.54 1.36Z"/></svg>
                    <strong>Settings</strong>
                </button>
            </nav>

            <section class="workspace-grid">
                <div class="glass-panel" id="library">
                    <header>
                        <div>
                            <p>Audio Library</p>
                            <h2>Tracks</h2>
                        </div>
                        <button class="panel-icon-button" data-reset="library" data-destructive="true" type="button" title="Delete library">
                            <svg viewBox="0 0 24 24" aria-hidden="true"><path d="M3 6h18M8 6V4h8v2m-9 4 1 10h8l1-10"/></svg>
                        </button>
                    </header>
                    <div id="trackList" class="track-list"></div>
                </div>

                <div class="glass-panel" id="playlists">
                    <header>
                        <div>
                            <p>Playlists</p>
                            <h2>Playlists</h2>
                        </div>
                    </header>
                    <form id="playlistForm" class="inline-form">
                        <input name="name" placeholder="New playlist name" required maxlength="255">
                        <button type="submit">Create</button>
                    </form>
                    <div id="playlistList" class="playlist-list"></div>
                </div>

                <div class="glass-panel" id="upload">
                    <header>
                        <div>
                            <p>Upload</p>
                            <h2>MP3 file</h2>
                        </div>
                    </header>
                    <form id="uploadForm" class="stack-form">
                        <input name="title" placeholder="Track title optional">
                        <input name="file" type="file" accept=".mp3,audio/mpeg" required>
                        <button type="submit">Upload</button>
                    </form>
                </div>

                <div class="glass-panel" id="add-url">
                    <header>
                        <div>
                            <p>Add URL</p>
                            <h2>Direct stream</h2>
                        </div>
                    </header>
                    <form id="linkForm" class="stack-form">
                        <input name="title" placeholder="Stream name" required maxlength="255">
                        <input name="url" placeholder="Direct MP3 or radio stream URL" required>
                        <button type="submit">Add stream</button>
                    </form>
                </div>
            </section>
        </main>
    </div>

    <footer class="player-bar">
        <div class="bar-track">
            <span class="bar-art" aria-hidden="true">
                <svg viewBox="0 0 24 24"><path d="M9 17.5a3 3 0 1 1-2-2.83V6.8L18 4v10.7a3 3 0 1 1-2-2.83V7.2l-7 1.76z"/></svg>
            </span>
            <div>
                <strong id="barTitle">Ready to play</strong>
                <small id="barAction">Playback stopped</small>
            </div>
        </div>
        <label class="bar-volume" aria-label="Player volume">
            <span>Volume</span>
            <input id="barVolumeSlider" type="range" min="0" max="21" value="12">
            <output id="barVolumeValue">12</output>
        </label>
        <div class="bar-controls">
            <span class="bar-duration" id="barDuration">00:00</span>
            <button class="playback-toggle" id="barPlaybackToggle" type="button" aria-label="Play or pause">
                <svg class="icon-play" viewBox="0 0 24 24" aria-hidden="true"><path d="M8 5v14l11-7z"/></svg>
                <svg class="icon-pause" viewBox="0 0 24 24" aria-hidden="true"><path d="M8 5h3v14H8zM13 5h3v14h-3z"/></svg>
            </button>
        </div>
    </footer>

    <dialog id="innerEyeModal">
        <div class="modal-head">
            <div>
                <p>Telemetry</p>
                <h2>Device monitor</h2>
            </div>
            <button id="closeInnerEye" type="button">Close</button>
        </div>
        <div id="deviceList" class="device-list"></div>
    </dialog>

    <dialog id="settingsModal">
        <div class="modal-head">
            <div>
                <p>Settings</p>
                <h2>Reset controls</h2>
            </div>
            <button id="closeSettings" type="button">Close</button>
        </div>
        <div class="reset-grid">
            <button data-reset="playback" type="button">Reset Playback</button>
            <button data-reset="soft_device" type="button">Restart Device</button>
            <button data-reset="wifi_setup" type="button">Change WiFi</button>
            <button data-reset="factory_device" data-destructive="true" type="button">Factory Reset Device</button>
            <button data-reset="server" type="button">Reset Server State</button>
            <button data-reset="library" data-destructive="true" type="button">Clear Library</button>
            <button data-reset="full" data-destructive="true" type="button">Full System Reset</button>
        </div>
    </dialog>

    <div id="toast" class="toast" role="status" aria-live="polite"></div>
</body>
</html>
