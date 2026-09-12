const CACHE_NAME = 'auxiot-shell-v33';
const SHELL = [
    './',
    './index.php',
    './assets/app.css?v=33',
    './assets/app.js?v=33',
    './manifest.json',
    './icons/icon-192.png',
    './icons/icon-512.png'
];

self.addEventListener('install', (event) => {
    event.waitUntil(caches.open(CACHE_NAME).then((cache) => cache.addAll(SHELL)));
    self.skipWaiting();
});

self.addEventListener('activate', (event) => {
    event.waitUntil(
        caches.keys().then((keys) => Promise.all(keys.filter((key) => key !== CACHE_NAME).map((key) => caches.delete(key))))
    );
    self.clients.claim();
});

self.addEventListener('fetch', (event) => {
    const url = new URL(event.request.url);
    const blocked = url.pathname.endsWith('/api.php')
        || url.pathname.endsWith('/state.json')
        || url.pathname.includes('/uploads/')
        || url.searchParams.has('action');

    if (blocked) {
        event.respondWith(fetch(event.request, { cache: 'no-store' }));
        return;
    }

    if (event.request.method !== 'GET') {
        event.respondWith(fetch(event.request));
        return;
    }

    event.respondWith(
        caches.match(event.request).then((cached) => cached || fetch(event.request).then((response) => {
            if (response.ok) {
                const copy = response.clone();
                caches.open(CACHE_NAME).then((cache) => cache.put(event.request, copy));
            }
            return response;
        }))
    );
});
