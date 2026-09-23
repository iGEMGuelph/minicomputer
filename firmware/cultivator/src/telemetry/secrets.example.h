/*
  secrets.example.h  —  TEMPLATE for telemetry secrets.

  Only needed if the backend requires a key or uses https with a certificate.
  To use it: copy this file to secrets.h (same folder) and fill it in.
  secrets.h is gitignored — never commit real keys.

  If secrets.h does not exist, the defaults below are used (no key, and
  https URLs are accepted without checking the certificate).
*/

#ifndef TELEMETRY_SECRETS_H
#define TELEMETRY_SECRETS_H

// Sent in the API_KEY_HEADER header (see config.h). "" = don't send a header.
#define BACKEND_API_KEY   ""

// Only for https:// BACKEND_URLs. Paste the server's root CA certificate
// (PEM text) to have the board verify the server. Leave it commented out and
// the connection is still encrypted, but the server's identity is not checked.
// #define BACKEND_ROOT_CA  "-----BEGIN CERTIFICATE-----\n" \
//                          "...\n" \
//                          "-----END CERTIFICATE-----\n"

#endif  // TELEMETRY_SECRETS_H
