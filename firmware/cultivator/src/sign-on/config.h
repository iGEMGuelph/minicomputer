/*
  config.h  —  non-secret sign-on settings.
  Your actual login goes in secrets.h instead (gitignored, not this file).
*/

#ifndef SIGNON_CONFIG_H
#define SIGNON_CONFIG_H

// Network name. eduroam is almost always literally "eduroam" — only change
// this if your institution uses a different SSID for the same login.
#define EDUROAM_SSID   "eduroam"

// How long to wait for authentication before giving up (milliseconds).
#define WIFI_CONNECT_TIMEOUT_MS   20000

// After authenticating, we try to open a plain TCP connection to this
// host/port as a real "can we reach the internet" test (successful 802.1X
// auth doesn't always mean traffic is actually flowing). Point TEST_HOST at
// your interface/API server once you know its address, or leave the default
// to just confirm general internet reachability.
#define TEST_HOST   "example.com"
#define TEST_PORT   443

// How often the loop() prints a status line once connected (milliseconds).
#define STATUS_PRINT_INTERVAL_MS   10000

#endif  // SIGNON_CONFIG_H
