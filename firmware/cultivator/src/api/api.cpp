/* ===========================================================================
   api.cpp  —  READ_ONLY flag + the small web server the dashboard talks to
   ---------------------------------------------------------------------------
   The dashboard backend sends GET /api/status; we reply with the device name
   and the READ_ONLY state. READ_ONLY is fixed at build time from secrets.h
   (READ_ONLY_MODE): change it by editing that file and re-flashing, never at
   runtime or over the network.

   The ESP32 core's WebServer.h doesn't exist on the GIGA R1 (mbed core), so
   this parses the one request line it needs by hand on top of WiFiServer /
   WiFiClient, which both boards' WiFi libraries provide.
   =========================================================================== */

#include <Arduino.h>
#include <WiFi.h>
#include "../sign-on/secrets.h"   // WiFi login + READ_ONLY_MODE (git-ignored)

#ifndef READ_ONLY_MODE
#error "secrets.h must define READ_ONLY_MODE as true or false"
#endif

// --- Settings ---------------------------------------------------------------
#define API_SERVER_PORT    80            // plain HTTP, so URLs need no port
#define API_DEVICE_NAME    "cultivator"  // lets the backend check it reached us

// Fixed at build time; only changes by editing secrets.h and re-flashing.
static const bool READ_ONLY = READ_ONLY_MODE;

static WiFiServer server(API_SERVER_PORT);
static bool serverRunning = false;

// Serves one client: reads just the request line, replies to GET /api/status
// with e.g. {"device":"cultivator","readOnly":true}, otherwise 404.
static void serveClient(WiFiClient& client) {
  String requestLine = client.readStringUntil('\n');
  // Drain the rest of the request (headers/body) so the client sees a clean close.
  while (client.connected() && client.available()) client.read();

  if (requestLine.startsWith("GET /api/status")) {
    String body = String("{\"device\":\"") + API_DEVICE_NAME +
                  "\",\"readOnly\":" + (READ_ONLY ? "true" : "false") + "}";
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
    server.stop();
    serverRunning = false;
    Serial.println(F("API stopped (WiFi down)."));
  }

  if (serverRunning) {
    WiFiClient client = server.available();
    if (client) serveClient(client);
  }
}
