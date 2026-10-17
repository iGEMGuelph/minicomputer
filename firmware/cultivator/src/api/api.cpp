/* ===========================================================================
   api.cpp  —  the small web server the dashboard talks to
   ---------------------------------------------------------------------------
   The dashboard backend sends GET /api/status to check the board is online;
   we reply with the device name and whether it is read-only (lights in
   MANUAL mode at the serial console, so the dashboard isn't in charge).
   Everything else (light levels, readings) the board sends OUT itself: see
   src/backend/.

   The ESP32 core's WebServer.h doesn't exist on the GIGA R1 (mbed core), so
   this parses the one request line it needs by hand on top of WiFiServer /
   WiFiClient, which both boards' WiFi libraries provide.
   =========================================================================== */

#include <Arduino.h>
#include <WiFi.h>
#include "../sign-on/secrets.h"   // WiFi login + DEVICE_SHARED_KEY (git-ignored)
#include "../cultivator/cultivator.h"

#ifndef DEVICE_SHARED_KEY
#error "secrets.h must define DEVICE_SHARED_KEY (the API auth key)"
#endif

// --- Settings ---------------------------------------------------------------
#define API_SERVER_PORT    80            // plain HTTP, so URLs need no port
#define API_DEVICE_NAME    "cultivator"  // lets the backend check it reached us
#define API_KEY_HEADER     "X-Device-Key" // request header carrying DEVICE_SHARED_KEY
#define API_MAX_HEADERS    32            // stop reading after this many header lines

static WiFiServer server(API_SERVER_PORT);
static bool serverRunning = false;

// Compares every character regardless of where the first mismatch is, so
// response timing doesn't reveal how much of a guessed key was right.
static bool keyMatches(const String& given) {
  const char* expected = DEVICE_SHARED_KEY;
  size_t expectedLen = strlen(expected);
  uint8_t diff = (given.length() == expectedLen) ? 0 : 1;
  for (size_t i = 0; i < expectedLen; i++) {
    char c = (i < given.length()) ? given[i] : 0;
    diff |= c ^ expected[i];
  }
  return diff == 0;
}

// Reads header lines up to the blank line; returns true if one of them is
// API_KEY_HEADER with the right key. Header names are case-insensitive.
static bool readHeadersAndCheckKey(WiFiClient& client) {
  bool authorized = false;
  for (int i = 0; i < API_MAX_HEADERS; i++) {
    String line = client.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) break;   // end of headers

    int colon = line.indexOf(':');
    if (colon < 0) continue;
    String name = line.substring(0, colon);
    name.trim();
    if (name.equalsIgnoreCase(API_KEY_HEADER)) {
      String value = line.substring(colon + 1);
      value.trim();
      authorized = keyMatches(value);
    }
  }
  return authorized;
}

// Serves one client: replies to GET /api/status with e.g.
// {"device":"cultivator","readOnly":false}, otherwise 404. Every request must
// carry API_KEY_HEADER: DEVICE_SHARED_KEY, or it gets 401 before routing.
static void serveClient(WiFiClient& client) {
  String requestLine = client.readStringUntil('\n');
  bool authorized = readHeadersAndCheckKey(client);
  // Drain anything left (e.g. a body) so the client sees a clean close.
  while (client.connected() && client.available()) client.read();

  if (!authorized) {
    client.println(F("HTTP/1.1 401 Unauthorized"));
    client.println(F("Connection: close"));
    client.println();
  } else if (requestLine.startsWith("GET /api/status")) {
    bool readOnly = (currentMode() == MODE_MANUAL);
    String body = String("{\"device\":\"") + API_DEVICE_NAME +
                  "\",\"readOnly\":" + (readOnly ? "true" : "false") + "}";
    client.println(F("HTTP/1.1 200 OK"));
    client.println(F("Content-Type: application/json"));
    client.print  (F("Content-Length: ")); client.println(body.length());
    client.println(F("Connection: close"));
    client.println();
    client.print(body);
  } else {
    client.println(F("HTTP/1.1 404 Not Found"));
    client.println(F("Connection: close"));
    client.println();
  }
  client.stop();
}

void apiSetup() {
}

void apiLoop() {
  bool wifiUp = (WiFi.status() == WL_CONNECTED);

  // Listen only while WiFi is up; restart cleanly after a reconnect.
  if (wifiUp && !serverRunning) {
    server.begin();
    serverRunning = true;
    Serial.print(F("API listening: http://"));
    Serial.print(WiFi.localIP());
    Serial.println(F("/api/status"));
  } else if (!wifiUp && serverRunning) {
    server.end();
    serverRunning = false;
    Serial.println(F("API stopped (WiFi down)."));
  }

  if (serverRunning) {
    WiFiClient client = server.accept();
    if (client) serveClient(client);
  }
}
