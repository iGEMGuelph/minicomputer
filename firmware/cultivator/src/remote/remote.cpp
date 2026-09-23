/* ===========================================================================
   remote.cpp  —  follow the dashboard's dimmer sliders over WiFi

       ESP32  --GET  /api/getLightDims-->      backend  (what should the lights be?)
       ESP32  --POST /api/reportLightState-->  backend  (here is what I am doing)

   Mode interaction:
     * A successful poll moves AUTO -> REMOTE.
     * MANUAL is left alone: someone at the serial console wins. The dashboard
       shows "read only" until they type "remote".
     * If the backend goes unreachable the lights HOLD; only after
       REMOTE_STALE_TIMEOUT_MS do they fall back to the RTC schedule.
   =========================================================================== */

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include "config.h"
#include "../cultivator/cultivator.h"

static const char* REMOTE_FW_VERSION = "0.3.0";

static unsigned long lastPollAt      = 0;
static unsigned long lastSuccessAt   = 0;
static unsigned long lastErrorLogAt  = 0;
static bool          everConnected   = false;
static long          lastRevision    = -1;   // -1 = nothing applied yet
static bool          staleWarned     = false;

static String baseUrl() {
  return String("http://") + API_HOST + ":" + String(API_PORT) + API_PREFIX;
}

// GET the desired levels and apply them. True if the backend gave a usable answer.
static bool pollLightDims() {
  HTTPClient http;
  http.setTimeout(HTTP_TIMEOUT_MS);
  http.setConnectTimeout(HTTP_TIMEOUT_MS);

  if (!http.begin(baseUrl() + "/getLightDims")) return false;

  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    if (code > 0) {
      Serial.print(F("[remote] getLightDims returned HTTP ")); Serial.println(code);
    }
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    Serial.print(F("[remote] could not parse response: ")); Serial.println(err.c_str());
    return false;
  }

  long revision = doc["revision"] | -1;

  int levels[CH_COUNT];
  for (int ch = 0; ch < CH_COUNT; ch++) {
    levels[ch] = doc[channelName(ch)] | 0;
  }
  setAllLevels(levels);

  if (currentMode() == MODE_AUTO) {
    Serial.println(F("[remote] backend reachable — switching to REMOTE mode."));
    setMode(MODE_REMOTE);
  }

  if (revision != lastRevision) {
    lastRevision = revision;
    Serial.print(F("[remote] applied revision ")); Serial.print(revision);
    Serial.print(F(":"));
    for (int ch = 0; ch < CH_COUNT; ch++) {
      Serial.print(' '); Serial.print(channelName(ch));
      Serial.print('='); Serial.print(levels[ch]);
      if (!channelIsWired(ch)) Serial.print(F("(unwired)"));
    }
    Serial.println();
  }
  return true;
}

// POST what we are actually doing. Best effort; a failure must not affect the lights.
static void reportState() {
  JsonDocument doc;
  doc["revision"] = lastRevision;
  doc["mode"]     = modeName(currentMode());
  doc["firmware"] = REMOTE_FW_VERSION;
  doc["rtc_ok"]   = rtcPresent();
  doc["rssi"]     = (int)WiFi.RSSI();

  JsonObject applied = doc["applied"].to<JsonObject>();
  JsonObject wired   = doc["wired"].to<JsonObject>();
  for (int ch = 0; ch < CH_COUNT; ch++) {
    applied[channelName(ch)] = channelLevel(ch);
    wired[channelName(ch)]   = channelIsWired(ch);
  }

  String body;
  serializeJson(doc, body);

  HTTPClient http;
  http.setTimeout(HTTP_TIMEOUT_MS);
  http.setConnectTimeout(HTTP_TIMEOUT_MS);
  if (!http.begin(baseUrl() + "/reportLightState")) return;
  http.addHeader("Content-Type", "application/json");
  http.POST(body);
  http.end();
}

// ===========================================================================
// SETUP  &  LOOP
// ===========================================================================
void remoteSetup() {
  Serial.println();
  Serial.print(F("[remote] dashboard at ")); Serial.println(baseUrl());
  Serial.println(F("[remote] waiting for WiFi before polling..."));
}

void remoteLoop() {
  if (WiFi.status() != WL_CONNECTED) return;

  if (millis() - lastPollAt < POLL_INTERVAL_MS) return;
  lastPollAt = millis();

  if (pollLightDims()) {
    lastSuccessAt = millis();
    if (!everConnected) {
      everConnected = true;
      Serial.println(F("[remote] first successful poll — dashboard is in control."));
    }
    staleWarned = false;
    reportState();
    return;
  }

  if (millis() - lastErrorLogAt >= POLL_ERROR_LOG_INTERVAL_MS) {
    lastErrorLogAt = millis();
    Serial.print(F("[remote] cannot reach the backend at ")); Serial.println(baseUrl());
    Serial.println(F("         Check API_HOST in src/remote/config.h, and that the"));
    Serial.println(F("         backend machine is on this same WiFi network."));
  }

  if (everConnected && currentMode() == MODE_REMOTE &&
      millis() - lastSuccessAt > REMOTE_STALE_TIMEOUT_MS) {
    if (!staleWarned) {
      staleWarned = true;
      Serial.println(F("[remote] backend unreachable too long — reverting to the"));
      Serial.println(F("         AUTO daily schedule. Will resume when it returns."));
    }
    setMode(MODE_AUTO);
  }
}
