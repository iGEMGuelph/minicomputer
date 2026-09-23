/*
  secrets.example.h  —  TEMPLATE for the WiFi login.

  Copy this file to secrets.h (same folder) and fill in real values.
  secrets.h is gitignored — never commit real passwords.
*/

#ifndef SIGNON_SECRETS_H
#define SIGNON_SECRETS_H

// Private WiFi (tried first). Plain WPA2 network name + password.
#define NOAH_IDENTITY   "your-wifi-name"
#define NOAH_PASSWORD   "your-wifi-password"

// eduroam (fallback). See src/sign-on/config.h for the SSID.
#define EAP_IDENTITY    "you@uoguelph.ca"
#define EAP_USERNAME    "you@uoguelph.ca"
#define EAP_PASSWORD    "your-eduroam-password"

#endif  // SIGNON_SECRETS_H
