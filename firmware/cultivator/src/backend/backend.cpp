/* ===========================================================================
   backend.cpp  —  outbound calls from the board to the dashboard backend
   ---------------------------------------------------------------------------
       board --GET  /api/getLightDims-->      backend  (what should the lights be?)
       board --POST /api/reportLightState-->  backend  (here is what I am doing)
       board --POST /api/logReading-->        backend  (one RTC-stamped reading)

   Every request carries DEVICE_KEY_HEADER: DEVICE_SHARED_KEY (from secrets.h);
   the backend answers 401 without it.

   Mode interaction (lights):
     * A successful poll moves AUTO -> REMOTE: the dashboard takes over.
     * MANUAL is left alone: someone at the serial console wins, and the
       backend shows "read only" until they type "remote" or "auto".
     * If the backend goes unreachable the lights HOLD; only after
       REMOTE_STALE_TIMEOUT_MS do they fall back to the AUTO schedule.

   Readings:
     * Taken every READING_INTERVAL_MS from the RTC + sensors.h, queued, and
       sent oldest first. A reading stays queued until the backend answers
       201, so nothing is lost while WiFi or the backend is down (up to
       READING_QUEUE_LEN). Without an RTC no readings are taken: the backend
       rejects a reading with no time.

   Like api.cpp, this speaks HTTP by hand over WiFiClient, because the ESP32
   HTTPClient library doesn't exist on the GIGA R1's mbed core. Settings live
   in config.h. All logging goes to the Serial Monitor only.
   =========================================================================== */

#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>

#include "config.h"
#include "sensors.h"
#include "../cultivator/cultivator.h"
#include "../sign-on/secrets.h"   // DEVICE_SHARED_KEY (git-ignored)

#ifndef DEVICE_SHARED_KEY
#error "secrets.h must define DEVICE_SHARED_KEY (the API auth key)"
#endif

static const char* FIRMWARE_VERSION = "0.3.0";

// The backend has four channels; this board only has RED and BLUE wired.
// UVB and FAR_RED are always reported as unwired and 0.
static const char* const CHANNELS[] = { "uvb", "red", "far_red", "blue" };

// --- Lights state ------------------------------------------------------------
static unsigned long nextPollAt     = 0;
static unsigned long lastSuccessAt  = 0;
static bool          everConnected  = false;
static long          lastRevision   = -1;   // -1 = nothing applied yet
static bool          staleWarned    = false;

// --- Readings queue ----------------------------------------------------------
struct Reading {
  char  time[24];                  // "2026-09-23T14:05:00" (Toronto wall clock)
  float ph, biomass, airTemp, waterTemp;   // NAN = no reading -> JSON null
};
static Reading       queue[READING_QUEUE_LEN];
static int           queueHead     = 0;    // index of the oldest reading
static int           queueCount    = 0;
static unsigned long nextReadingAt = 0;
static unsigned long nextSendAt    = 0;
static bool          warnedNoClock = false;

static unsigned long lastErrorLogAt = 0;
static bool          errorLoggedOnce = false;

// ===========================================================================
// HTTP  —  one request per connection, "Connection: close"
// ===========================================================================

// Sends one request to API_PREFIX + path. Returns the HTTP status code, or -1
// if the backend couldn't be reached or the reply was garbled. If respBody is
// given, the response body is stored there.
static int httpRequest(const char* method, const char* path,
                       const String& reqBody, String* respBody) {
  WiFiClient client;
  client.setTimeout(HTTP_TIMEOUT_MS);
  if (!client.connect(BACKEND_HOST, BACKEND_PORT)) return -1;

  client.print(method); client.print(' ');
  client.print(F(API_PREFIX)); client.print(path);
  client.println(F(" HTTP/1.1"));
  client.print(F("Host: ")); client.print(F(BACKEND_HOST));
  client.print(':'); client.println(BACKEND_PORT);
  client.print(F(DEVICE_KEY_HEADER ": ")); client.println(F(DEVICE_SHARED_KEY));
  client.println(F("Connection: close"));
  if (reqBody.length() > 0) {
    client.println(F("Content-Type: application/json"));
    client.print(F("Content-Length: ")); client.println(reqBody.length());
  }
  client.println();
  if (reqBody.length() > 0) client.print(reqBody);

  // Status line, e.g. "HTTP/1.1 201 Created"
  String statusLine = client.readStringUntil('\n');
  int sp = statusLine.indexOf(' ');
  int code = (sp > 0) ? statusLine.substring(sp + 1).toInt() : 0;
  if (code <= 0) { client.stop(); return -1; }

  // Headers: only Content-Length matters, so we know when the body ends.
  long contentLength = -1;
  for (;;) {
    String line = client.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) break;
    int colon = line.indexOf(':');
    if (colon > 0 && line.substring(0, colon).equalsIgnoreCase("Content-Length")) {
      contentLength = line.substring(colon + 1).toInt();
    }
  }

  if (respBody) {
    *respBody = "";
    unsigned long start = millis();
    while ((contentLength < 0 || (long)respBody->length() < contentLength) &&
           millis() - start < HTTP_TIMEOUT_MS) {
      if (client.available())       *respBody += (char)client.read();
      else if (!client.connected()) break;
    }
  }
  client.stop();
  return code;
}

// Rate-limited "can't reach the backend" message.
static void logUnreachable(const char* what, int code) {
  if (errorLoggedOnce && millis() - lastErrorLogAt < BACKEND_ERROR_LOG_INTERVAL_MS) return;
  errorLoggedOnce = true;
  lastErrorLogAt = millis();
  Serial.print(F("[backend] ")); Serial.print(what);
  if (code < 0) {
    Serial.print(F(": cannot reach http://"));
    Serial.print(F(BACKEND_HOST)); Serial.print(':'); Serial.println(BACKEND_PORT);
    Serial.println(F("          Check BACKEND_HOST/BACKEND_PORT in src/backend/config.h"));
    Serial.println(F("          and that the backend is on this same network."));
  } else {
    Serial.print(F(" returned HTTP ")); Serial.println(code);
    if (code == 401) {
      Serial.println(F("          DEVICE_SHARED_KEY in secrets.h doesn't match the backend's."));
    }
  }
}

// ===========================================================================
// LIGHTS  —  getLightDims + reportLightState
// ===========================================================================

// GET the dashboard's levels and hand them to the cultivator.
// Returns the HTTP code (200 = applied).
static int pollLightDims() {
  String body;
  int code = httpRequest("GET", "/getLightDims", "", &body);
  if (code != 200) return code;

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    Serial.print(F("[backend] getLightDims: could not parse reply: "));
    Serial.println(err.c_str());
    return -1;
  }

  long revision = doc["revision"] | -1L;
  int  red      = doc["red"]  | 0;
  int  blue     = doc["blue"] | 0;
  setRemoteLevels(red, blue);

  if (currentMode() == MODE_AUTO) {
    Serial.println(F("[backend] dashboard reachable - switching to REMOTE mode."));
    setMode(MODE_REMOTE);
  }

  if (revision != lastRevision) {
    lastRevision = revision;
    Serial.print(F("[backend] dashboard levels, revision ")); Serial.print(revision);
    Serial.print(F(": red=")); Serial.print(red);
    Serial.print(F(" blue=")); Serial.print(blue);
    int uvb = doc["uvb"] | 0, farRed = doc["far_red"] | 0;
    if (uvb || farRed) {
      Serial.print(F(" (uvb=")); Serial.print(uvb);
      Serial.print(F(" far_red=")); Serial.print(farRed);
      Serial.print(F(" ignored: not wired)"));
    }
    if (currentMode() == MODE_MANUAL) Serial.print(F(" [MANUAL mode: not applied]"));
    Serial.println();
  }
  return code;
}

// POST what the lights are actually doing. Expected answer: 204.
static int reportLightState() {
  JsonDocument doc;
  doc["revision"] = lastRevision;
  doc["mode"]     = modeName(currentMode());
  doc["firmware"] = FIRMWARE_VERSION;
  doc["rtc_ok"]   = rtcPresent();
  doc["rssi"]     = (int)WiFi.RSSI();

  JsonObject applied = doc["applied"].to<JsonObject>();
  JsonObject wired   = doc["wired"].to<JsonObject>();
  for (const char* ch : CHANNELS) {
    bool isRed = strcmp(ch, "red") == 0, isBlue = strcmp(ch, "blue") == 0;
    applied[ch] = isRed ? lightRed() : isBlue ? lightBlue() : 0;
    wired[ch]   = isRed || isBlue;
  }

  String body;
  serializeJson(doc, body);
  return httpRequest("POST", "/reportLightState", body, nullptr);
}

static void lightsLoop() {
  if ((long)(millis() - nextPollAt) < 0) return;

  int code = pollLightDims();
  if (code == 200) {
    lastSuccessAt = millis();
    if (!everConnected) {
      everConnected = true;
      Serial.println(F("[backend] first successful poll - dashboard connected."));
    }
    staleWarned = false;
    int reportCode = reportLightState();
    if (reportCode != 204) logUnreachable("reportLightState", reportCode);
    nextPollAt = millis() + LIGHTS_POLL_INTERVAL_MS;
    return;
  }

  logUnreachable("getLightDims", code);
  nextPollAt = millis() + (code < 0 ? BACKEND_RETRY_INTERVAL_MS : LIGHTS_POLL_INTERVAL_MS);

  if (everConnected && currentMode() == MODE_REMOTE &&
      millis() - lastSuccessAt > REMOTE_STALE_TIMEOUT_MS) {
    if (!staleWarned) {
      staleWarned = true;
      Serial.println(F("[backend] dashboard unreachable too long - reverting to the"));
      Serial.println(F("          AUTO daily schedule. Will resume when it returns."));
    }
    setMode(MODE_AUTO);
  }
}

// ===========================================================================
// READINGS  —  logReading
// ===========================================================================

// Take a reading and queue it; if the queue is full, drop the oldest.
static void takeReading() {
  DateTime now;
  if (!cultivatorNow(now)) {
    if (!warnedNoClock) {
      warnedNoClock = true;
      Serial.println(F("[backend] RTC not found - no readings will be sent."));
    }
    return;
  }
  warnedNoClock = false;

  if (queueCount == READING_QUEUE_LEN) {
    queueHead = (queueHead + 1) % READING_QUEUE_LEN;
    queueCount--;
    Serial.println(F("[backend] reading buffer full - dropped the oldest."));
  }
  Reading& r = queue[(queueHead + queueCount) % READING_QUEUE_LEN];
  // No UTC offset: the backend reads a bare time as Toronto wall-clock time.
  snprintf(r.time, sizeof(r.time), "%04d-%02d-%02dT%02d:%02d:%02d",
           now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
  r.ph        = readPH();
  r.biomass   = readBiomass();
  r.airTemp   = readAirTemp();
  r.waterTemp = readWaterTemp();
  queueCount++;
}

static void setNumberOrNull(JsonDocument& doc, const char* key, float value) {
  if (isnan(value) || isinf(value)) doc[key] = nullptr;
  else                              doc[key] = value;
}

// POST one reading. Expected answer: 201.
static int sendReading(const Reading& r, String& body) {
  JsonDocument doc;
  doc["Time"] = r.time;
  setNumberOrNull(doc, "pH",        r.ph);
  setNumberOrNull(doc, "Biomass",   r.biomass);
  setNumberOrNull(doc, "AirTemp",   r.airTemp);
  setNumberOrNull(doc, "WaterTemp", r.waterTemp);
  body = "";
  serializeJson(doc, body);
  return httpRequest("POST", "/logReading", body, nullptr);
}

static void readingsLoop() {
  if ((long)(millis() - nextReadingAt) >= 0) {
    nextReadingAt = millis() + READING_INTERVAL_MS;
    takeReading();
  }

  if (queueCount == 0 || (long)(millis() - nextSendAt) < 0) return;

  String body;
  int code = sendReading(queue[queueHead], body);
#if BACKEND_LOG_EACH_READING
  Serial.print(F("[backend] POST /logReading ")); Serial.print(body);
  Serial.print(F(" -> ")); Serial.println(code);
#endif

  if (code == 201 || code == 422) {
    // 201: stored. 422: the backend rejected the data itself, and resending
    // the same reading would never succeed, so drop it either way.
    if (code == 422) Serial.println(F("[backend] reading rejected (422) - dropped."));
    queueHead = (queueHead + 1) % READING_QUEUE_LEN;
    queueCount--;
    nextSendAt = millis();   // send the next queued one on the next loop()
  } else {
    // Unreachable, 401, or 500 (not stored): keep it and try again later.
    logUnreachable("logReading", code);
    nextSendAt = millis() + BACKEND_RETRY_INTERVAL_MS;
  }
}

// ===========================================================================
// SETUP  &  LOOP
// ===========================================================================
void backendSetup() {
  Serial.println();
  Serial.print(F("[backend] dashboard at http://"));
  Serial.print(F(BACKEND_HOST)); Serial.print(':'); Serial.print(BACKEND_PORT);
  Serial.println(F(API_PREFIX));
  Serial.println(F("[backend] waiting for WiFi..."));
  nextPollAt = nextReadingAt = nextSendAt = millis();
}

void backendLoop() {
  if (WiFi.status() != WL_CONNECTED) {
    // Readings are still taken (and queued) while WiFi is down.
    if ((long)(millis() - nextReadingAt) >= 0) {
      nextReadingAt = millis() + READING_INTERVAL_MS;
      takeReading();
    }
    return;
  }
  lightsLoop();
  readingsLoop();
}
