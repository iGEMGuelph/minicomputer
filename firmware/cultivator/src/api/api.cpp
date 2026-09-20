/* ===========================================================================
   api.cpp  —  READ_ONLY flag + the small web server the dashboard talks to
   ---------------------------------------------------------------------------
   The dashboard backend sends GET /api/status; we reply with the device name
   and the READ_ONLY state. READ_ONLY is fixed at build time from secrets.h
   (READ_ONLY_MODE): change it by editing that file and re-flashing, never at
   runtime or over the network.
   =========================================================================== */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "../sign-on/secrets.h"   // WiFi login + READ_ONLY_MODE (git-ignored)

#ifndef READ_ONLY_MODE
#error "secrets.h must define READ_ONLY_MODE as true or false"
#endif

// --- Settings ---------------------------------------------------------------
#define API_SERVER_PORT    80            // plain HTTP, so URLs need no port
#define API_DEVICE_NAME    "cultivator"  // lets the backend check it reached us

// Fixed at build time; only changes by editing secrets.h and re-flashing.
static const bool READ_ONLY = READ_ONLY_MODE;

static WebServer server(API_SERVER_PORT);
static bool serverRunning = false;

// Answers GET /api/status, e.g. {"device":"cultivator","readOnly":true}
static void handleStatus() {
  String body = String("{\"device\":\"") + API_DEVICE_NAME +
                "\",\"readOnly\":" + (READ_ONLY ? "true" : "false") + "}";
  server.send(200, "application/json", body);
}

void apiSetup() {
  server.on("/api/status", HTTP_GET, handleStatus);
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

  if (serverRunning) server.handleClient();   // serve any waiting request
}
