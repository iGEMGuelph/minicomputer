/*
  config.h  —  telemetry (data upload) settings.

  Everything about "what gets sent, where, and how often" lives here.
  Edit a value, save, re-upload. You should not need to touch telemetry.cpp.

  Secrets (an API key, if the backend ever needs one) do NOT go here —
  they go in secrets.h (gitignored). See secrets.example.h.

  Full explanation of the data flow: docs/TELEMETRY.md
*/

#ifndef TELEMETRY_CONFIG_H
#define TELEMETRY_CONFIG_H

// ===========================================================================
// 1) ON / OFF
// ===========================================================================
// 1 = take a reading every TELEMETRY_INTERVAL_MS and upload it.
// 0 = telemetry completely disabled (grow lights keep working as normal).
// Can also be paused at runtime with the serial command "telemetry off".
#define TELEMETRY_ENABLED        1

// ===========================================================================
// 2) WHERE TO SEND IT  —  the backend endpoint
// ===========================================================================
// Full URL of the backend route that receives one reading as JSON (HTTP POST).
// The board and the backend must be on the same network (or the backend must
// be publicly reachable). "localhost" will NOT work here — localhost on the
// board means the board itself. Use the backend computer's IP address, e.g.
//   http://192.168.1.42:8000/api/logReading
// Find it with `ipconfig getifaddr en0` (Mac) or `ipconfig` (Windows).
// https:// URLs are supported too (see BACKEND_ROOT_CA in secrets.example.h).
#define BACKEND_URL              "http://192.168.1.100:8000/api/logReading"

// ===========================================================================
// 3) HOW OFTEN
// ===========================================================================
#define TELEMETRY_INTERVAL_MS    10000   // one reading every 10 seconds

// How long to wait for the backend to answer before giving up on a reading.
// Keep this well under TELEMETRY_INTERVAL_MS.
#define HTTP_TIMEOUT_MS          5000

// Readings waiting to be sent (e.g. while WiFi is reconnecting).
// When full, the OLDEST waiting reading is dropped to make room.
// 30 x 10 s = 5 minutes of buffering.
#define TELEMETRY_QUEUE_LEN      30

// ===========================================================================
// 4) THE JSON  —  field names and decimal places
// ===========================================================================
// These key names are the contract with the backend. Change them here only if
// the backend changes too.
#define KEY_TIME        "Time"
#define KEY_PH          "pH"
#define KEY_BIOMASS     "Biomass"
#define KEY_AIR_TEMP    "AirTemp"
#define KEY_WATER_TEMP  "WaterTemp"

// Decimal places sent for each value (the backend re-validates anyway).
#define DECIMALS_PH           1    // e.g. 7.2
#define DECIMALS_BIOMASS      2    // proportion 0-1, e.g. 0.35
#define DECIMALS_AIR_TEMP     2    // degrees C, e.g. 22.50
#define DECIMALS_WATER_TEMP   2    // degrees C, e.g. 19.75

// The RTC stores plain clock time with no timezone. This text is stuck on the
// end of every timestamp so the backend knows how to read it:
//   ""        -> "2026-09-23T16:14:00"        (no timezone info; the default)
//   "Z"       -> "2026-09-23T16:14:00Z"       (only if the RTC is set to UTC)
//   "-04:00"  -> "2026-09-23T16:14:00-04:00"  (Guelph in summer, EDT)
#define TIME_UTC_OFFSET   ""

// ===========================================================================
// 5) OPTIONAL AUTH HEADER
// ===========================================================================
// If the backend requires a key, put the key in secrets.h (BACKEND_API_KEY)
// and it will be sent in this header. An empty key = no header sent.
#define API_KEY_HEADER    "X-API-Key"

// ===========================================================================
// 6) ADVANCED  —  rarely changed
// ===========================================================================
// 1 = print every POST and its result on the Serial Monitor (handy while
// testing). 0 = only print failures.
#define TELEMETRY_LOG_EACH_SEND   1

#define TELEMETRY_JSON_BUF_SIZE   256    // bytes for one JSON message
#define UPLOAD_TASK_STACK         8192   // bytes (HTTPS needs the extra room)
#define UPLOAD_TASK_PRIORITY      1      // same as loop(); lights never wait on it

#endif  // TELEMETRY_CONFIG_H
