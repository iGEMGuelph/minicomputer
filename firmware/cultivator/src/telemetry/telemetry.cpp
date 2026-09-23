/* ===========================================================================
   telemetry.cpp  —  take a reading every 10 s and POST it to the backend
   ---------------------------------------------------------------------------
   HOW IT WORKS (two halves, so the grow lights never wait on the network):

     loop()  ──every TELEMETRY_INTERVAL_MS──►  take a Reading (RTC + sensors)
                                                   │
                                                   ▼
                                          FreeRTOS queue (buffer)
                                                   │
     upload task (runs on its own) ◄───────────────┘
        waits for a reading, POSTs it as JSON to BACKEND_URL

   * Sampling happens in loop() with a millis() timer — it takes microseconds,
     so the lights/serial menu are unaffected, and all hardware access (RTC,
     future sensors) stays on the main loop like the rest of the sketch.
   * Uploading happens in a separate FreeRTOS task. An HTTP request can take
     seconds (slow WiFi, backend down); doing it in its own task means loop()
     never blocks. The task sleeps (0% CPU) until a reading arrives.
   * If WiFi is down, readings wait in the queue and are sent once it is back.
     If the queue fills up, the oldest reading is dropped.
   * WiFi itself is handled by the sign-on module (src/sign-on/). This module
     only checks whether WiFi is connected; it never connects/disconnects.

   Settings: config.h     Sensors: sensors.h     Docs: docs/TELEMETRY.md
   =========================================================================== */

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <string.h>
#include <RTClib.h>

#include "config.h"
#include "reading.h"
#include "sensors.h"

#if __has_include("secrets.h")
  #include "secrets.h"
#else
  #include "secrets.example.h"   // defaults: no API key
#endif

// Provided by the cultivator module, which owns the RTC.
bool cultivatorNow(DateTime& out);

static QueueHandle_t readingQueue = nullptr;
static volatile bool telemetryActive = (TELEMETRY_ENABLED != 0);
static unsigned long nextSampleAt = 0;
static bool warnedNoClock = false;

// Counters for the "net" command. Written by one task, read by the other;
// 32-bit values are atomic on the ESP32, so no lock is needed for these.
static volatile uint32_t statQueued  = 0;   // readings taken
static volatile uint32_t statSent    = 0;   // backend answered 2xx
static volatile uint32_t statFailed  = 0;   // error / non-2xx answer
static volatile uint32_t statDropped = 0;   // buffer full, oldest thrown away
static volatile int      lastHttpCode = 0;  // last answer (negative = no connection)

// ===========================================================================
// SAMPLING  (main loop)
// ===========================================================================

// Build a Reading from the RTC and sensors. Returns false if there is no clock.
static bool takeReading(Reading& r) {
  r = emptyReading();

  DateTime now;
  if (!cultivatorNow(now)) return false;
  r.hasTime = true;
  r.year   = now.year();   r.month  = now.month();  r.day    = now.day();
  r.hour   = now.hour();   r.minute = now.minute(); r.second = now.second();

  r.ph        = readPH();
  r.biomass   = readBiomass();
  r.airTemp   = readAirTemp();
  r.waterTemp = readWaterTemp();
  return true;
}

// Put a reading in the queue; if full, drop the oldest to make room.
static void enqueue(const Reading& r) {
  if (xQueueSend(readingQueue, &r, 0) != pdTRUE) {
    Reading oldest;
    xQueueReceive(readingQueue, &oldest, 0);
    xQueueSend(readingQueue, &r, 0);
    statDropped++;
  }
  statQueued++;
}

// Take a reading and queue it. A reading without a time is useless to the
// backend, so if the RTC is missing we skip it (and say so once).
static bool sampleNow() {
  Reading r;
  if (!takeReading(r)) {
    if (!warnedNoClock) {
      Serial.println(F("[telemetry] RTC not found - no readings will be sent."));
      warnedNoClock = true;
    }
    return false;
  }
  warnedNoClock = false;
  enqueue(r);
  return true;
}

// ===========================================================================
// UPLOADING  (separate FreeRTOS task)
// ===========================================================================

// POST one reading. Returns the HTTP status code, or a negative HTTPClient
// error code if the backend could not be reached.
static int postReading(const Reading& r) {
  char body[TELEMETRY_JSON_BUF_SIZE];
  int len = buildReadingJson(r, body, sizeof(body));
  if (len < 0) {
    Serial.println(F("[telemetry] JSON too big - raise TELEMETRY_JSON_BUF_SIZE"));
    return -1000;
  }

  static WiFiClient       plainClient;
  static WiFiClientSecure secureClient;
  bool https = strncmp(BACKEND_URL, "https://", 8) == 0;
  if (https) {
#ifdef BACKEND_ROOT_CA
    secureClient.setCACert(BACKEND_ROOT_CA);
#else
    secureClient.setInsecure();   // encrypted, but server identity not checked
#endif
  }

  HTTPClient http;
  if (!(https ? http.begin(secureClient, BACKEND_URL)
              : http.begin(plainClient,  BACKEND_URL))) {
    return -1001;   // bad URL
  }
  http.setConnectTimeout(HTTP_TIMEOUT_MS);
  http.setTimeout(HTTP_TIMEOUT_MS);
  http.addHeader("Content-Type", "application/json");
  if (strlen(BACKEND_API_KEY) > 0) {
    http.addHeader(API_KEY_HEADER, BACKEND_API_KEY);
  }

  int code = http.POST((uint8_t*)body, len);
  http.end();

#if TELEMETRY_LOG_EACH_SEND
  Serial.printf("[telemetry] POST %s -> %d\n", body, code);
#endif
  return code;
}

static void uploadTask(void*) {
  for (;;) {
    // Leave readings in the queue until WiFi is up, so nothing is lost
    // during a short dropout.
    if (WiFi.status() != WL_CONNECTED) {
      vTaskDelay(pdMS_TO_TICKS(1000));
      continue;
    }

    Reading r;
    if (xQueueReceive(readingQueue, &r, pdMS_TO_TICKS(1000)) != pdTRUE) {
      continue;   // nothing to send yet; loop round and re-check WiFi
    }

    int code = postReading(r);
    lastHttpCode = code;
    if (code >= 200 && code < 300) {
      statSent++;
    } else {
      // Not retried: the next reading is only 10 s away, and retrying would
      // let a broken backend pile up work. The failure is counted and logged.
      statFailed++;
      if (code < 0) {
        Serial.printf("[telemetry] backend unreachable (%s) at %s\n",
                      HTTPClient::errorToString(code).c_str(), BACKEND_URL);
      } else {
        Serial.printf("[telemetry] backend answered HTTP %d\n", code);
      }
    }
  }
}

// ===========================================================================
// SERIAL COMMANDS  (called from the cultivator module's command parser)
// ===========================================================================
void telemetryPrintHelp() {
  Serial.println(F("  net                  show WiFi + upload status"));
  Serial.println(F("  payload              print the JSON a reading would send"));
  Serial.println(F("  send                 take a reading now and upload it"));
  Serial.println(F("  telemetry <on|off>   resume / pause uploads"));
}

static void printTelemetryStatus() {
  Serial.println(F("---- TELEMETRY ----"));
  Serial.print(F("  uploads   : "));
  if (!TELEMETRY_ENABLED)       Serial.println(F("DISABLED in telemetry/config.h"));
  else if (!telemetryActive)    Serial.println(F("paused ('telemetry on' to resume)"));
  else { Serial.print(F("every ")); Serial.print(TELEMETRY_INTERVAL_MS / 1000); Serial.println(F(" s")); }
  Serial.print(F("  backend   : ")); Serial.println(BACKEND_URL);
  Serial.print(F("  WiFi      : "));
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print(F("connected, IP ")); Serial.println(WiFi.localIP());
  } else {
    Serial.println(F("NOT connected (readings wait in the buffer)"));
  }
  Serial.printf("  readings  : %lu taken, %lu sent, %lu failed, %lu dropped\n",
                (unsigned long)statQueued, (unsigned long)statSent,
                (unsigned long)statFailed, (unsigned long)statDropped);
  Serial.printf("  waiting   : %u in buffer (max %d)\n",
                readingQueue ? (unsigned)uxQueueMessagesWaiting(readingQueue) : 0u,
                TELEMETRY_QUEUE_LEN);
  Serial.print(F("  last HTTP : "));
  if (lastHttpCode == 0)     Serial.println(F("nothing sent yet"));
  else if (lastHttpCode < 0) Serial.println(HTTPClient::errorToString(lastHttpCode));
  else                       Serial.println(lastHttpCode);
  Serial.println(F("-------------------"));
}

// Returns true if the command belonged to telemetry (handled here).
bool telemetryHandleCommand(const String& cmd, String tok[], int n) {
  if (cmd == "net") {
    printTelemetryStatus();
  } else if (cmd == "payload") {
    Reading r;
    takeReading(r);   // no clock -> "Time": null, shown as-is
    char body[TELEMETRY_JSON_BUF_SIZE];
    if (buildReadingJson(r, body, sizeof(body)) >= 0) Serial.println(body);
    else Serial.println(F("JSON too big - raise TELEMETRY_JSON_BUF_SIZE"));
  } else if (cmd == "send") {
    if (!readingQueue)     Serial.println(F("Telemetry is disabled in telemetry/config.h"));
    else if (sampleNow())  Serial.println(F("Reading queued - see 'net' for the result."));
  } else if (cmd == "telemetry" && n >= 2) {
    String arg = tok[1];
    arg.toLowerCase();
    if (!readingQueue) {
      Serial.println(F("Telemetry is disabled in telemetry/config.h"));
    } else if (arg == "on") {
      telemetryActive = true;
      nextSampleAt = millis();
      Serial.println(F("Telemetry ON"));
    } else if (arg == "off") {
      telemetryActive = false;
      Serial.println(F("Telemetry paused"));
    } else {
      Serial.println(F("Use: telemetry on | telemetry off"));
    }
  } else if (cmd == "telemetry") {
    printTelemetryStatus();
  } else {
    return false;
  }
  return true;
}

// ===========================================================================
// SETUP  &  LOOP
// ===========================================================================
void telemetrySetup() {
  sensorsSetup();

  if (!TELEMETRY_ENABLED) {
    Serial.println(F("[telemetry] disabled in telemetry/config.h"));
    return;
  }

  readingQueue = xQueueCreate(TELEMETRY_QUEUE_LEN, sizeof(Reading));
  if (!readingQueue ||
      xTaskCreate(uploadTask, "telemetry", UPLOAD_TASK_STACK, nullptr,
                  UPLOAD_TASK_PRIORITY, nullptr) != pdPASS) {
    Serial.println(F("[telemetry] could not start (out of memory) - uploads off"));
    telemetryActive = false;
    return;
  }

  Serial.print(F("[telemetry] every "));
  Serial.print(TELEMETRY_INTERVAL_MS / 1000);
  Serial.print(F(" s to "));
  Serial.println(BACKEND_URL);
  nextSampleAt = millis();   // first reading right away
}

void telemetryLoop() {
  if (!readingQueue || !telemetryActive) return;

  // Signed difference handles millis() wrap-around (every ~49 days).
  if ((long)(millis() - nextSampleAt) < 0) return;

  // Schedule from the previous target (not "now") so the 10 s rhythm does
  // not slowly drift. If we fell far behind, resync instead of bursting.
  nextSampleAt += TELEMETRY_INTERVAL_MS;
  if ((long)(millis() - nextSampleAt) >= 0) nextSampleAt = millis() + TELEMETRY_INTERVAL_MS;

  sampleNow();
}
