/*
  config.h  —  where the dashboard backend is, and how often to talk to it.

  THE SETTINGS YOU WILL ACTUALLY CHANGE ARE BACKEND_HOST AND BACKEND_PORT.
  The values below are PLACEHOLDERS.

  The board reaches OUT to the backend (it is the HTTP client here), so it
  works on networks that block incoming connections and nobody has to track
  the board's address for these calls. The shared key comes from secrets.h
  (DEVICE_SHARED_KEY), the same one api.cpp checks on incoming requests.
*/

#ifndef BACKEND_CONFIG_H
#define BACKEND_CONFIG_H

// ===========================================================================
// 1) WHERE THE BACKEND IS  —  PLACEHOLDERS, fill in before flashing
// ===========================================================================
// IP address of the computer running the backend, on the SAME network as this
// board. Not "localhost" (that would mean the board itself). Find it with
// `ipconfig` (Windows) or `hostname -I` (Linux/Mac), then check that
// http://<ip>:<port><API_PREFIX>/health loads from a phone on that network.
#define BACKEND_HOST   "192.168.1.100"   // PLACEHOLDER
#define BACKEND_PORT   8000              // PLACEHOLDER (uvicorn's default)

// Must match API_PREFIX in the backend (pathways/api.py).
#define API_PREFIX     "/api"            // PLACEHOLDER

// Must match DEVICE_KEY_HEADER in the backend.
#define DEVICE_KEY_HEADER  "X-Device-Key"

// ===========================================================================
// 2) HOW OFTEN
// ===========================================================================
// Ask for the dashboard's slider levels, then report what the lights are
// actually doing. The report doubles as the heartbeat: the backend shows
// "disconnected" if it stops arriving.
#define LIGHTS_POLL_INTERVAL_MS   2000

// Upload one sensor reading to /logReading.
#define READING_INTERVAL_MS       10000

// Every request blocks loop() until it finishes, so keep this short.
#define HTTP_TIMEOUT_MS           2000

// When the backend can't be reached, wait this long between attempts instead
// of the normal intervals, so a dead backend doesn't keep stalling loop().
#define BACKEND_RETRY_INTERVAL_MS 15000

// If the backend is unreachable this long while in REMOTE mode, fall back to
// the RTC schedule (AUTO). Until then the lights HOLD their last levels.
#define REMOTE_STALE_TIMEOUT_MS   (5UL * 60UL * 1000UL)

// Readings kept for retry while the backend is down. When full, the OLDEST
// is dropped. 30 x 10 s = 5 minutes of buffering.
#define READING_QUEUE_LEN         30

// ===========================================================================
// 3) LOGGING  (Serial Monitor only, nothing is sent anywhere)
// ===========================================================================
// 1 = print every reading upload and its HTTP result. 0 = only failures.
// Light polls are printed only when the dashboard's levels change.
#define BACKEND_LOG_EACH_READING  1

// Minimum gap between repeated "cannot reach backend" messages.
#define BACKEND_ERROR_LOG_INTERVAL_MS 30000

#endif  // BACKEND_CONFIG_H
