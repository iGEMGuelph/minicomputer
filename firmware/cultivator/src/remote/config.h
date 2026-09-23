/*
  config.h  —  dashboard connection settings.

  THE ONE SETTING YOU WILL ACTUALLY CHANGE IS API_HOST.

  The board does not host a server: it reaches OUT to the backend every couple
  of seconds and asks what the lights should be. Outbound works on networks that
  block device-to-device traffic, and nobody has to track the board's address.
*/

#ifndef REMOTE_CONFIG_H
#define REMOTE_CONFIG_H

// IP address of the computer running the backend, on the SAME WiFi network as
// this board. Not "localhost". Find it with `hostname -I` (Linux/Mac) or
// `ipconfig` (Windows), then confirm from a phone on that WiFi that
// http://<ip>:8000/api/health loads.
//
// Campus WiFi (eduroam) blocks devices from reaching each other — use a phone
// hotspot. If the lights never respond, this line is the first thing to check.
#define API_HOST   "10.70.1.150"

// From the dashboard repo's shared/config.json. Change only if that changes.
#define API_PORT   8000
#define API_PREFIX "/api"

#define POLL_INTERVAL_MS        2000   // how often to ask for new levels
#define HTTP_TIMEOUT_MS         2000   // HTTPClient blocks loop() while waiting; keep short

// If the backend is unreachable this long, fall back to the RTC schedule.
// Until then the lights HOLD their last levels — stale beats dark for a grow light.
#define REMOTE_STALE_TIMEOUT_MS (5UL * 60UL * 1000UL)

#define POLL_ERROR_LOG_INTERVAL_MS 30000   // rate-limit "cannot reach backend" messages

#endif  // REMOTE_CONFIG_H
