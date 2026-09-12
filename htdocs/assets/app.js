const api = (action) => `api.php?action=${encodeURIComponent(action)}&t=${Date.now()}`;
const state = { busy: false, tracks: [], playlists: [], deviceTimer: null, lastDevices: [], isPlaying: false, playlistId: null, trackDuration: null, currentTrackId: null, currentAction: 'stop', discStartX: null, networkRisk: {}, lastNetworkToastAt: 0, toastTimer: null, toastStartX: null };

const $ = (selector) => document.querySelector(selector);
const trackList = $('#trackList');
const playlistList = $('#playlistList');
const toast = $('#toast');

document.addEventListener('DOMContentLoaded', () => {
    initTheme();
    bindControls();
    registerServiceWorker();
    refreshAll();
    loadDeviceSummary();
    setInterval(() => {
        loadState().catch(() => {});
    }, 2500);
    setInterval(() => {
        loadDeviceSummary().catch(() => {});
    }, 2000);
});

function bindControls() {
    document.body.addEventListener('click', async (event) => {
        const control = event.target.closest('[data-control]');
        const reset = event.target.closest('[data-reset]');
        const play = event.target.closest('[data-play-track]');
        const deleteTrack = event.target.closest('[data-delete-track]');
        const renameTrack = event.target.closest('[data-rename-track]');
        const playPlaylist = event.target.closest('[data-play-playlist]');
        const discPlaylist = event.target.closest('[data-disc-playlist]');
        const playlistNav = event.target.closest('[data-playlist-nav]');
        const removePlaylistTrack = event.target.closest('[data-remove-playlist-track]');

        const actionTarget = control || reset || play || deleteTrack || renameTrack || playPlaylist || discPlaylist || playlistNav || removePlaylistTrack;
        if (actionTarget) {
            event.preventDefault();
            event.stopPropagation();
        }

        if (control) {
            await guarded(() => postJson('control', { control: control.dataset.control }));
            return;
        }
        if (play) {
            await guarded(() => postJson('control', { control: 'play', track_id: play.dataset.playTrack }));
            closeActionSurfaces();
            return;
        }
        if (deleteTrack) {
            await guarded(async () => {
            if (!confirm('Are you sure?')) return;
            await postJson('delete_track', { track_id: deleteTrack.dataset.deleteTrack });
            await refreshAll();
            });
            closeActionSurfaces();
            return;
        }
        if (renameTrack) {
            await renameLibraryTrack(renameTrack.dataset.renameTrack, renameTrack.dataset.currentTitle || '');
            closeActionSurfaces();
            return;
        }
        if (playPlaylist) {
            await guarded(() => postJson('play_playlist', { playlist_id: playPlaylist.dataset.playPlaylist }));
            return;
        }
        if (discPlaylist) {
            await playDiscPlaylist(discPlaylist.dataset.discPlaylist);
            return;
        }
        if (playlistNav) {
            await navigatePlaylist(playlistNav.dataset.playlistNav);
            return;
        }
        if (removePlaylistTrack) {
            await guarded(async () => {
            await postJson('playlist_remove', { playlist_track_id: removePlaylistTrack.dataset.removePlaylistTrack });
            await loadPlaylists();
            });
            closeActionSurfaces();
            return;
        }
        if (reset) await guarded(() => runReset(reset));
    });

    if ($('#volumeSlider')) bindVolumeControl($('#volumeSlider'));
    bindVolumeControl($('#barVolumeSlider'));
    $('#heroPlaybackToggle').addEventListener('click', () => handlePlaybackToggle());
    $('#barPlaybackToggle').addEventListener('click', () => handlePlaybackToggle());
    $('#discPickerToggle')?.addEventListener('click', (event) => {
        event.preventDefault();
        event.stopPropagation();
        $('#turntable').classList.toggle('show-picker');
    });
    bindTurntable();
    bindSwipeActions();
    bindToastDismiss();

    if ($('#autoPlayToggle')) {
        $('#autoPlayToggle').addEventListener('change', (event) => {
            guarded(() => postJson('control', { control: 'auto_play', auto_play: event.target.checked }));
        });
    }

    $('#uploadForm').addEventListener('submit', (event) => {
        event.preventDefault();
        const form = new FormData(event.target);
        guarded(async () => {
            const response = await apiFetch('upload', { method: 'POST', body: form });
            await handleResponse(response);
            event.target.reset();
            await refreshAll();
        });
    });

    $('#linkForm').addEventListener('submit', (event) => {
        event.preventDefault();
        const payload = Object.fromEntries(new FormData(event.target));
        guarded(async () => {
            await postJson('add_link', payload);
            event.target.reset();
            await refreshAll();
        });
    });

    $('#playlistForm').addEventListener('submit', (event) => {
        event.preventDefault();
        const payload = Object.fromEntries(new FormData(event.target));
        guarded(async () => {
            await postJson('playlist_create', payload);
            event.target.reset();
            await refreshAll();
        });
    });

    $('#deviceQuickButton').addEventListener('click', () => openInnerEye());
    $('#settingsButton').addEventListener('click', () => openSettings());
    $('#themeToggle').addEventListener('click', () => toggleTheme());
    $('#settingsQuickButton').addEventListener('click', () => openSettings());
    $('#settingsDockButton').addEventListener('click', () => openSettings());
    $('#deviceStatusButton').addEventListener('click', () => openInnerEye());
    $('#closeInnerEye').addEventListener('click', () => closeInnerEye());
    $('#innerEyeModal').addEventListener('close', () => closeInnerEye());
    $('#closeSettings').addEventListener('click', () => closeSettings());
    $('#settingsModal').addEventListener('close', () => closeSettings());
}

function initTheme() {
    const saved = localStorage.getItem('auxiot-theme');
    const prefersLight = window.matchMedia?.('(prefers-color-scheme: light)').matches;
    setTheme(saved || (prefersLight ? 'light' : 'dark'));
}

function toggleTheme() {
    setTheme(document.documentElement.dataset.theme === 'light' ? 'dark' : 'light');
}

function setTheme(theme) {
    const next = theme === 'light' ? 'light' : 'dark';
    document.documentElement.dataset.theme = next;
    localStorage.setItem('auxiot-theme', next);
    document.querySelector('meta[name="theme-color"]')?.setAttribute('content', next === 'light' ? '#f5f7fb' : '#07080d');
    const label = next === 'light' ? 'Dark mode' : 'Light mode';
    $('#themeToggle')?.setAttribute('title', label);
    $('#themeToggle')?.setAttribute('aria-label', label);
}

function bindToastDismiss() {
    toast.addEventListener('pointerdown', (event) => {
        state.toastStartX = event.clientX;
    });
    toast.addEventListener('pointerup', (event) => {
        if (state.toastStartX === null) return;
        const delta = event.clientX - state.toastStartX;
        state.toastStartX = null;
        if (Math.abs(delta) > 42) hideToast();
    });
}

function bindSwipeActions() {
    let activeRow = null;
    let startX = 0;

    document.body.addEventListener('pointerdown', (event) => {
        const row = event.target.closest('.swipe-card');
        if (!row || event.target.closest('button, summary, select, input, a')) return;
        activeRow = row;
        startX = event.clientX;
    });

    document.body.addEventListener('pointerup', (event) => {
        if (!activeRow) return;
        const delta = event.clientX - startX;
        if (delta < -36) {
            document.querySelectorAll('.swipe-card.is-open').forEach((row) => {
                if (row !== activeRow) row.classList.remove('is-open');
            });
            activeRow.classList.add('is-open');
        } else if (delta > 24) {
            activeRow.classList.remove('is-open');
        }
        activeRow = null;
    });

    document.body.addEventListener('click', (event) => {
        if (!event.target.closest('.swipe-card, .action-menu')) {
            closeActionSurfaces();
        }
    });
}

function closeActionSurfaces() {
    document.querySelectorAll('.swipe-card.is-open').forEach((row) => row.classList.remove('is-open'));
    document.querySelectorAll('.action-menu[open]').forEach((menu) => menu.removeAttribute('open'));
}

async function guarded(work) {
    if (state.busy) return;
    state.busy = true;
    setBusy(true);
    try {
        await work();
        await loadState();
    } catch (error) {
        showError(error);
    } finally {
        state.busy = false;
        setBusy(false);
    }
}

function setBusy(isBusy) {
    document.querySelectorAll('button, input, select').forEach((node) => {
        if (!['closeInnerEye', 'closeSettings', 'deviceStatusButton', 'deviceQuickButton', 'settingsButton', 'settingsQuickButton', 'settingsDockButton'].includes(node.id)) node.disabled = isBusy;
    });
}

async function refreshAll() {
    await Promise.all([loadState(), loadLibrary(), loadPlaylists()]);
}

async function loadState() {
    const data = await getJson('state');
    const current = data.state;
    state.isPlaying = Boolean(current.is_playing || current.action === 'play');
    state.currentAction = current.action || 'stop';
    state.playlistId = current.playlist_id ? Number(current.playlist_id) : null;
    state.currentTrackId = current.track_id ? Number(current.track_id) : null;
    state.trackDuration = Number.isFinite(Number(current.duration_seconds)) ? Number(current.duration_seconds) : null;
    $('#currentTitle').textContent = current.title || 'Nothing playing';
    $('#currentMeta').textContent = friendlyStateLabel(current);
    $('#barTitle').textContent = current.title || 'Ready to play';
    $('#barAction').textContent = playbackLabel(current);
    updateHeroHint(current);
    updateDurationUi();
    setVolumeUi(current.volume ?? 12);
    if ($('#autoPlayToggle')) $('#autoPlayToggle').checked = Boolean(current.auto_play);
    setPlaybackToggleUi(state.isPlaying);
    setTurntableUi(state.isPlaying);
    updateSeaWave(current);
}

async function navigatePlaylist(direction) {
    if (!state.playlistId) {
        $('#turntable').classList.add('show-picker');
        showToast('Choose a playlist on the disc first.');
        return;
    }
    await guarded(() => postJson('playlist_nav', { direction }));
    await loadPlaylists();
}

function bindTurntable() {
    const disc = $('#turntable');
    const picker = $('#discPlaylistPicker');
    picker?.addEventListener('pointerdown', (event) => event.stopPropagation());
    picker?.addEventListener('pointerup', (event) => event.stopPropagation());
    disc.addEventListener('wheel', (event) => {
        if (event.target.closest('.disc-playlist-picker')) return;
        event.preventDefault();
        navigatePlaylist(event.deltaY > 0 || event.deltaX > 0 ? 'next' : 'previous');
    }, { passive: false });
    disc.addEventListener('pointerdown', (event) => {
        if (event.target.closest('button, .disc-playlist-picker')) return;
        state.discStartX = event.clientX;
        disc.setPointerCapture?.(event.pointerId);
    });
    disc.addEventListener('pointerup', (event) => {
        if (event.target.closest('button, .disc-playlist-picker')) return;
        if (state.discStartX === null) return;
        const delta = event.clientX - state.discStartX;
        state.discStartX = null;
        if (Math.abs(delta) < 34) return;
        navigatePlaylist(delta > 0 ? 'next' : 'previous');
    });
}

async function playDiscPlaylist(playlistId) {
    if (!playlistId) return;
    $('#turntable').classList.remove('show-picker');
    await guarded(() => postJson('play_playlist', { playlist_id: playlistId }));
    await loadPlaylists();
}

function setTurntableUi(isPlaying) {
    const disc = $('#turntable');
    disc.classList.toggle('is-spinning', isPlaying);
    disc.classList.toggle('show-picker', !state.playlistId);
    $('#playbackInsight').classList.toggle('is-playing', isPlaying);
}

function updateDurationUi() {
    const label = durationLabel();
    $('#heroDuration').textContent = label;
    $('#barDuration').textContent = label;
}

function durationLabel() {
    if (state.trackDuration !== null && state.trackDuration > 0) {
        return formatDuration(state.trackDuration);
    }
    if (state.currentTrackId && state.isPlaying) {
        return 'LIVE';
    }
    return '00:00';
}

function formatDuration(seconds) {
    const safe = Math.max(0, Math.floor(Number(seconds) || 0));
    const hours = Math.floor(safe / 3600);
    const minutes = Math.floor((safe % 3600) / 60);
    const secs = safe % 60;
    if (hours > 0) {
        return `${hours}:${String(minutes).padStart(2, '0')}:${String(secs).padStart(2, '0')}`;
    }
    return `${String(minutes).padStart(2, '0')}:${String(secs).padStart(2, '0')}`;
}

function renderDiscPlaylistPicker() {
    const picker = $('#discPlaylistPicker');
    if (!picker) return;
    const playable = state.playlists.filter((playlist) => (playlist.tracks || []).length > 0);
    if (!playable.length) {
        picker.innerHTML = '<small>Add tracks to a playlist first</small>';
        return;
    }
    picker.innerHTML = `
        <small>Choose playlist</small>
        <div>
            ${playable.map((playlist) => `
                <button type="button" data-disc-playlist="${playlist.id}" class="${Number(playlist.id) === state.playlistId ? 'is-selected' : ''}">
                    ${escapeHtml(playlist.name)}
                </button>
            `).join('')}
        </div>
    `;
}

async function renameLibraryTrack(trackId, currentTitle) {
    const title = prompt('Rename track', currentTitle) || '';
    if (!title.trim()) return;
    await guarded(async () => {
        await postJson('rename_track', { track_id: trackId, title });
        await refreshAll();
    });
}

async function handlePlaybackToggle() {
    if (!state.isPlaying) {
        showToast('Choose a track or playlist first.');
        return;
    }
    await guarded(() => postJson('control', { control: 'pause' }));
}

function setPlaybackToggleUi(isPlaying) {
    document.querySelectorAll('.playback-toggle').forEach((button) => {
        button.classList.toggle('is-playing', isPlaying);
        button.setAttribute('aria-label', isPlaying ? 'Pause playback' : 'Choose a track to play');
        button.title = isPlaying ? 'Pause playback' : 'Choose a track to play';
    });
}

function bindVolumeControl(slider) {
    slider.addEventListener('input', (event) => setVolumeUi(event.target.value));
    slider.addEventListener('change', (event) => {
        guarded(() => postJson('control', { control: 'volume', volume: event.target.value }));
    });
}

function setVolumeUi(value) {
    const volume = Math.max(0, Math.min(21, Number(value) || 0));
    if ($('#volumeSlider')) $('#volumeSlider').value = volume;
    $('#barVolumeSlider').value = volume;
    $('#barVolumeValue').textContent = volume;
    updateSeaWave({ volume, is_playing: state.isPlaying, track_id: state.currentTrackId, title: $('#currentTitle').textContent });
}

function updateSeaWave(current = {}) {
    const wave = $('#seaWave');
    if (!wave) return;
    const isPlaying = Boolean(current.is_playing || current.action === 'play' || state.isPlaying);
    const volume = Math.max(0, Math.min(21, Number(current.volume) || Number($('#barVolumeSlider')?.value) || 0));
    const title = `${current.track_id || state.currentTrackId || ''}:${current.title || ''}`;
    const hash = [...title].reduce((sum, char) => (sum + char.charCodeAt(0)) % 97, 0);
    const level = isPlaying ? 0.34 + (volume / 21) * 0.58 : 0.16;
    const speed = isPlaying ? Math.max(4.8, 11.5 - (volume / 21) * 5.4 - (hash % 7) * 0.18) : 17;
    wave.classList.toggle('is-live', isPlaying);
    wave.style.setProperty('--wave-level', level.toFixed(2));
    wave.style.setProperty('--wave-speed', `${speed.toFixed(2)}s`);
    wave.style.setProperty('--wave-shift', `${(hash % 9) - 4}px`);
}

function playbackLabel(current) {
    if (current.is_playing || current.action === 'play') return 'Streaming now';
    if (current.action === 'volume') return 'Volume updated';
    if (current.action === 'auto_play') return current.auto_play ? 'Auto-play enabled' : 'Auto-play paused';
    if (current.action === 'soft_reset') return 'Device restarting';
    if (current.action === 'wifi_setup') return 'WiFi setup mode';
    if (current.action === 'factory_reset') return 'Device provisioning';
    return 'Playback stopped';
}

function friendlyStateLabel(current) {
    if (current.is_playing || current.action === 'play') return state.playlistId ? 'Playlist is playing' : 'Track is playing';
    if (current.action === 'volume') return 'Volume saved';
    if (current.action === 'pause' || current.action === 'stop') return 'Stopped';
    if (current.action === 'soft_reset') return 'Device restarting';
    if (current.action === 'wifi_setup') return 'WiFi setup mode';
    if (current.action === 'factory_reset') return 'Provisioning mode';
    return 'Ready';
}

function updateHeroHint(current) {
    const hint = $('#heroHint');
    if (!hint) return;
    if (current.is_playing || current.action === 'play') {
        hint.textContent = state.playlistId ? 'Rotate or swipe the disc to move through this playlist.' : 'Use the player bar to adjust volume or pause playback.';
        return;
    }
    if (state.tracks.length === 0) {
        hint.textContent = 'Add a direct stream or upload an MP3 to begin.';
        return;
    }
    if (state.playlists.some((playlist) => (playlist.tracks || []).length > 0)) {
        hint.textContent = 'Tap the playlist icon on the disc, or play any track from Library.';
        return;
    }
    hint.textContent = 'Play a track from Library, or add tracks into a playlist.';
}

async function loadLibrary() {
    const data = await getJson('library');
    state.tracks = data.tracks || [];
    trackList.innerHTML = state.tracks.length ? state.tracks.map(renderTrack).join('') : emptyMessage('No tracks yet', 'Upload an MP3 or add a radio stream to start your library.');
    updateHeroHint({ action: state.currentAction, is_playing: state.isPlaying });
}

async function loadPlaylists() {
    const data = await getJson('playlists');
    state.playlists = data.playlists || [];
    playlistList.innerHTML = state.playlists.length ? state.playlists.map(renderPlaylist).join('') : emptyMessage('No playlists yet', 'Create one, then add tracks from your library.');
    renderDiscPlaylistPicker();
    updateHeroHint({ action: state.currentAction, is_playing: state.isPlaying });
}

async function loadDevices() {
    const data = await getJson('device_status');
    const devices = data.devices || [];
    state.lastDevices = devices;
    updateDeviceStatus(devices);
    $('#deviceList').innerHTML = devices.length ? devices.map(renderDevice).join('') : emptyMessage('No device telemetry yet.');
}

async function loadDeviceSummary() {
    try {
        const data = await getJson('device_status');
        const devices = data.devices || [];
        state.lastDevices = devices;
        updateDeviceStatus(devices);
        if ($('#innerEyeModal').open) {
            $('#deviceList').innerHTML = devices.length ? devices.map(renderDevice).join('') : emptyMessage('No device telemetry yet.');
        }
    } catch (error) {
        updateDeviceStatus([], 'Telemetry error');
    }
}

function updateDeviceStatus(devices, fallback = 'Waiting for telemetry') {
    const pill = $('#deviceStatusButton');
    const innerEyeQuick = $('#deviceQuickButton');
    const text = $('#deviceStatusText');
    const meta = $('#deviceStatusMeta');
    pill.classList.remove('status-online-bg', 'status-stale-bg', 'status-offline-bg');
    innerEyeQuick.classList.remove('status-online-bg', 'status-stale-bg', 'status-offline-bg');

    if (!devices.length) {
        pill.classList.add('status-offline-bg');
        innerEyeQuick.classList.add('status-offline-bg');
        text.textContent = 'Device';
        meta.textContent = fallback;
        updateHeroHint({ action: state.isPlaying ? 'play' : 'stop', is_playing: state.isPlaying });
        return;
    }

    const rank = { online: 0, stale: 1, offline: 2 };
    const sorted = [...devices].sort((a, b) => rank[a.online_state] - rank[b.online_state]);
    const primary = sorted[0];
    const label = primary.online_state.charAt(0).toUpperCase() + primary.online_state.slice(1);
    const health = networkHealth(primary);
    pill.classList.add(`status-${primary.online_state}-bg`);
    innerEyeQuick.classList.add(`status-${primary.online_state}-bg`);
    text.textContent = `Device · ${label}`;
    meta.textContent = `${devices.length} device${devices.length === 1 ? '' : 's'} · ${formatAge(primary.age_seconds)} ago`;
    maybeNotifyNetwork(primary, health);
}

function networkHealth(device) {
    const rssi = Number(device.rssi);
    const age = Number(device.age_seconds) || 0;
    const error = String(device.last_error || '').toLowerCase();
    const wifi = String(device.wifi_status || '').toLowerCase();
    const reasons = [];

    if (Number.isFinite(rssi) && rssi <= -88) {
        reasons.push(`Very weak WiFi signal (${rssi} dBm)`);
    }
    if (age > 45 || device.online_state === 'offline') reasons.push('Device telemetry is offline');
    if (wifi && wifi !== 'connected') reasons.push('WiFi is not connected');
    if (/(wifi|timeout|offline|unable to open stream)/i.test(error)) reasons.push(`Network error: ${device.last_error}`);

    return {
        slow: reasons.length > 0,
        severity: reasons.some((reason) => /very weak|offline|not connected/i.test(reason)) ? 'critical' : 'warning',
        message: reasons[0] || '',
        detail: reasons.join(' · '),
    };
}

function maybeNotifyNetwork(device, health) {
    const deviceId = device.device_id || 'default';
    const isActivelyPlaying = device.playback_state === 'playing' || (device.action === 'play' && Number(device.age_seconds) < 8);
    const sample = state.networkRisk[deviceId] || { count: 0 };

    if (!health.slow || !isActivelyPlaying || health.severity !== 'critical') {
        state.networkRisk[deviceId] = { count: 0 };
        return;
    }

    sample.count += 1;
    state.networkRisk[deviceId] = sample;
    if (sample.count < 4) return;

    const now = Date.now();
    if (now - state.lastNetworkToastAt < 600000) return;
    state.lastNetworkToastAt = now;
    showToast('Device network looks unstable. Check Device Monitor when playback is affected.', 4200);
}

function formatAge(seconds) {
    const value = Math.max(0, Number(seconds) || 0);
    if (value < 60) return `${Math.floor(value)}s`;
    const minutes = Math.floor(value / 60);
    if (minutes < 60) return `${minutes}m`;
    const hours = Math.floor(minutes / 60);
    if (hours < 24) return `${hours}h`;
    const days = Math.floor(hours / 24);
    if (days < 7) return `${days}d`;
    const weeks = Math.floor(days / 7);
    if (weeks < 5) return `${weeks}w`;
    return `${Math.floor(days / 30)}mo`;
}

async function runReset(button) {
    const type = button.dataset.reset;
    const destructive = button.dataset.destructive === 'true';
    if (!confirm('Are you sure?')) return;
    let confirmText = '';
    if (destructive) {
        confirmText = prompt('Type RESET to confirm') || '';
        if (confirmText !== 'RESET') throw new Error('Reset cancelled.');
    }
    await postJson('reset', { type, confirm: confirmText });
    await refreshAll();
}

async function getJson(action) {
    const response = await apiFetch(action);
    return handleResponse(response);
}

async function postJson(action, payload) {
    const response = await apiFetch(action, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(payload),
    });
    const data = await handleResponse(response);
    showToast('Done');
    return data;
}

async function apiFetch(action, options = {}, retried = false) {
    const response = await fetch(api(action), {
        ...options,
        cache: 'no-store',
        credentials: 'same-origin',
    });

    if (!retried && await solveInfinityChallenge(response.clone())) {
        return apiFetch(action, options, true);
    }

    return response;
}

async function handleResponse(response) {
    const text = await response.text();
    let data;
    try {
        data = JSON.parse(text);
    } catch (error) {
        if (text.includes('slowAES.decrypt') || text.includes('__test')) {
            throw new Error('Security cookie refreshed. Try again.');
        }
        throw new Error('Server returned a non-JSON response.');
    }
    if (!response.ok || !data.ok) {
        throw new Error(data.error || 'Request failed.');
    }
    return data;
}

function showError(error) {
    showToast(error.message || 'Request failed.');
}

async function solveInfinityChallenge(response) {
    const type = response.headers.get('content-type') || '';
    if (type.includes('application/json')) return false;

    const text = await response.text();
    if (!text.includes('slowAES.decrypt') || !text.includes('__test')) return false;

    const values = [...text.matchAll(/toNumbers\("([0-9a-f]+)"\)/gi)].map((match) => match[1]);
    if (values.length < 3 || !window.crypto?.subtle) return false;

    try {
        const key = hexToBytes(values[0]);
        const iv = hexToBytes(values[1]);
        const cipher = hexToBytes(values[2]);
        const cryptoKey = await crypto.subtle.importKey('raw', key, { name: 'AES-CBC' }, false, ['decrypt']);
        const plain = await crypto.subtle.decrypt({ name: 'AES-CBC', iv }, cryptoKey, cipher);
        document.cookie = `__test=${bytesToHex(new Uint8Array(plain))}; max-age=21600; path=/; SameSite=Lax`;
        return true;
    } catch (error) {
        return false;
    }
}

function hexToBytes(hex) {
    const bytes = new Uint8Array(hex.length / 2);
    for (let index = 0; index < bytes.length; index += 1) {
        bytes[index] = parseInt(hex.slice(index * 2, index * 2 + 2), 16);
    }
    return bytes;
}

function bytesToHex(bytes) {
    return [...bytes].map((byte) => byte.toString(16).padStart(2, '0')).join('');
}

function renderTrack(track) {
    const title = escapeHtml(track.title);
    const duration = track.source_type === 'upload' && track.duration_seconds ? formatDuration(track.duration_seconds) : 'LIVE';
    return `
        <article class="track-card library-row">
            <div class="track-copy">
                <strong>${title}</strong>
                <small>${escapeHtml(track.source_type)} · ${escapeHtml(duration)}</small>
            </div>
            <div class="row-actions">
                <button class="icon-row-button is-primary" type="button" data-play-track="${track.id}" aria-label="Play ${title}" title="Play">${icon('play')}</button>
                <button class="icon-row-button" type="button" data-rename-track="${track.id}" data-current-title="${title}" aria-label="Rename ${title}" title="Rename">${icon('edit')}</button>
                <button class="icon-row-button is-danger" type="button" data-delete-track="${track.id}" aria-label="Delete ${title}" title="Delete">${icon('trash')}</button>
            </div>
        </article>
    `;
}

function renderPlaylist(playlist) {
    const options = state.tracks.map((track) => `<option value="${track.id}">${escapeHtml(track.title)}</option>`).join('');
    const count = (playlist.tracks || []).length;
    const tracks = (playlist.tracks || []).map((track) => `
        <div class="playlist-track library-row">
            <small>${escapeHtml(track.title)}</small>
            <div class="row-actions">
                <button class="icon-row-button is-primary" type="button" data-play-track="${track.id}" aria-label="Play ${escapeHtml(track.title)}" title="Play">${icon('play')}</button>
                <button class="icon-row-button is-danger" type="button" data-remove-playlist-track="${track.playlist_track_id}" aria-label="Remove ${escapeHtml(track.title)}" title="Remove">${icon('trash')}</button>
            </div>
        </div>
    `).join('');
    return `
        <article class="playlist-card">
            <div class="playlist-head">
                <div>
                    <strong>${escapeHtml(playlist.name)}</strong>
                    <small>${count} track${count === 1 ? '' : 's'}</small>
                </div>
                <button class="pill-action" type="button" data-play-playlist="${playlist.id}" ${count ? '' : 'disabled'}>${icon('play')}<span>Play</span></button>
            </div>
            <div>${tracks || '<div class="soft-empty"><small>No tracks in this playlist yet</small></div>'}</div>
            <form class="inline-form" onsubmit="return addTrackToPlaylist(event, ${playlist.id})">
                <select name="track_id" ${state.tracks.length ? '' : 'disabled'}>${options || '<option>Add tracks to library first</option>'}</select>
                <button type="submit" ${state.tracks.length ? '' : 'disabled'}>Add</button>
            </form>
        </article>
    `;
}

window.addTrackToPlaylist = function addTrackToPlaylist(event, playlistId) {
    event.preventDefault();
    const trackId = new FormData(event.target).get('track_id');
    guarded(async () => {
        await postJson('playlist_add', { playlist_id: playlistId, track_id: trackId });
        await loadPlaylists();
    });
    return false;
};

function renderDevice(device) {
    const health = networkHealth(device);
    const rows = [
        ['Device ID', device.device_id],
        ['IP address', device.ip_address],
        ['WiFi status', device.wifi_status],
        ['RSSI', device.rssi],
        ['Heap', device.free_heap],
        ['PSRAM', device.psram_status],
        ['Command ID', device.command_id],
        ['Action', device.action],
        ['Track title', device.track_title],
        ['Stream URL', device.stream_url],
        ['Volume', device.volume],
        ['Playback state', device.playback_state],
        ['Last error', device.last_error],
        ['Firmware', device.firmware_version],
        ['Uptime', device.uptime_ms],
        ['Last update', device.updated_at],
    ].map(([key, value]) => `<div><dt>${key}</dt><dd>${escapeHtml(value ?? '-')}</dd></div>`).join('');

    return `
        <article class="device-card">
            <strong class="status-${device.online_state}">${escapeHtml(device.online_state)}</strong>
            ${health.slow ? `
                <div class="network-warning ${health.severity === 'critical' ? 'is-critical' : ''}">
                    <strong>Connection attention</strong>
                    <small>${escapeHtml(health.detail || 'Device telemetry needs attention.')}</small>
                </div>
            ` : ''}
            <dl>${rows}</dl>
        </article>
    `;
}

function emptyMessage(title, detail = '') {
    return `<div class="empty-state"><strong>${escapeHtml(title)}</strong>${detail ? `<small>${escapeHtml(detail)}</small>` : ''}</div>`;
}

function actionMenu(contents, label) {
    return `
        <details class="action-menu">
            <summary aria-label="${label}">
                ${icon('more')}
            </summary>
            <div>${contents}</div>
        </details>
    `;
}

function icon(name) {
    const icons = {
        play: '<svg viewBox="0 0 24 24" aria-hidden="true"><path d="M8 5v14l11-7z"/></svg>',
        trash: '<svg viewBox="0 0 24 24" aria-hidden="true"><path d="M3 6h18M8 6V4h8v2m-9 4 1 10h8l1-10"/></svg>',
        edit: '<svg viewBox="0 0 24 24" aria-hidden="true"><path d="m4 16.5-.8 4.3 4.3-.8L19 8.5 15.5 5 4 16.5Z"/><path d="m14.5 6 3.5 3.5"/></svg>',
        more: '<svg viewBox="0 0 24 24" aria-hidden="true"><circle cx="12" cy="5" r="1.8"/><circle cx="12" cy="12" r="1.8"/><circle cx="12" cy="19" r="1.8"/></svg>',
    };
    return icons[name] || '';
}

function openInnerEye() {
    const modal = $('#innerEyeModal');
    if (!modal.open) modal.showModal();
    loadDevices();
    if (!state.deviceTimer) {
        state.deviceTimer = setInterval(loadDevices, 2000);
    }
}

function closeInnerEye() {
    if (state.deviceTimer) clearInterval(state.deviceTimer);
    state.deviceTimer = null;
    const modal = $('#innerEyeModal');
    if (modal.open) modal.close();
}

function openSettings() {
    const modal = $('#settingsModal');
    if (!modal.open) modal.showModal();
}

function closeSettings() {
    const modal = $('#settingsModal');
    if (modal.open) modal.close();
}

function showToast(message, duration = 1800) {
    toast.textContent = message;
    toast.classList.add('show');
    clearTimeout(state.toastTimer);
    state.toastTimer = setTimeout(hideToast, duration);
}

function hideToast() {
    toast.classList.remove('show');
}

function escapeHtml(value) {
    return String(value).replace(/[&<>"']/g, (char) => ({
        '&': '&amp;',
        '<': '&lt;',
        '>': '&gt;',
        '"': '&quot;',
        "'": '&#039;',
    }[char]));
}

function registerServiceWorker() {
    if ('serviceWorker' in navigator) {
        navigator.serviceWorker.register('service-worker.js').catch(() => {});
    }
}
