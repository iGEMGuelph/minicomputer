/*
  secrets.h  —  real login credentials. NEVER commit this file (it's in
  .gitignore). Edit the placeholder values below to match your network.
*/

#ifndef SECRETS_H
#define SECRETS_H

// Private WiFi (WPA2-PSK) — tried first before falling back to eduroam.
#define NOAH_IDENTITY   "YOUR_WIFI_USER"
#define NOAH_PASSWORD   "YOUR_WIFI_PASSWORD"

// Shared key every API request must carry in the X-Device-Key header.
// Must match DEVICE_SHARED_KEY in the dashboard backend. See README.md "API".
#define DEVICE_SHARED_KEY  "YOUR_SHARED_KEY"

#endif  // SECRETS_H
