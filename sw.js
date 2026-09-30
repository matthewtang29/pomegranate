// Service worker: lets the installed app open offline.
// Network first, and every request double-checks with GitHub that the file hasn't
// changed (instead of trusting the browser's 10-minute cache), so updates you push
// show up the next time the app opens. The cached copy is only used when offline.
const CACHE = 'pomodoro-v4';
const ASSETS = [
  './', 'index.html', 'style.css',
  'app.js', 'localtimer.js', 'score.js', 'analytics.js', 'tasks.js', 'calendar.js', 'journal.js', 'theme.js', 'main.js',
  'manifest.webmanifest', 'icons/icon.svg', 'icons/icon-192.png', 'icons/icon-512.png',
];

self.addEventListener('install', e => {
  e.waitUntil(caches.open(CACHE).then(c => c.addAll(ASSETS)).then(() => self.skipWaiting()));
});

self.addEventListener('activate', e => {
  e.waitUntil(caches.keys()
    .then(keys => Promise.all(keys.filter(k => k !== CACHE).map(k => caches.delete(k))))
    .then(() => self.clients.claim()));
});

self.addEventListener('fetch', e => {
  const req = e.request;
  if (req.method !== 'GET' || new URL(req.url).origin !== location.origin) return;
  e.respondWith(
    fetch(req.url, { cache: 'no-cache', credentials: 'same-origin' })
      .then(res => {
        if (res.ok) {
          const copy = res.clone();
          caches.open(CACHE).then(c => c.put(req, copy));
        }
        return res;
      })
      .catch(() => caches.match(req, { ignoreSearch: true })
        .then(hit => hit || (req.mode === 'navigate' ? caches.match('index.html') : undefined)))
  );
});
