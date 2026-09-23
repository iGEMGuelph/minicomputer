/*
  reading.h  —  one data point, and how it is turned into JSON.

  Pure C++ (NO Arduino dependencies), like cultivator/schedule.h, so it can be
  unit-tested on a normal computer:   pio test -e native
  (tests live in firmware/cultivator/test/test_reading/).

  A missing value is stored as NAN and sent as JSON null. That is how
  "this sensor doesn't exist yet / failed to read" reaches the backend —
  we never invent a number.
*/

#ifndef TELEMETRY_READING_H
#define TELEMETRY_READING_H

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stddef.h>
#include "config.h"

struct Reading {
  bool  hasTime;                         // false -> "Time": null
  int   year, month, day;                // from the RTC
  int   hour, minute, second;
  float ph;                              // 0-14,  NAN = no reading
  float biomass;                         // 0-1,   NAN = no reading
  float airTemp;                         // deg C, NAN = no reading
  float waterTemp;                       // deg C, NAN = no reading
};

// A Reading with no time and every sensor value missing.
inline Reading emptyReading() {
  Reading r = {};
  r.hasTime   = false;
  r.ph        = NAN;
  r.biomass   = NAN;
  r.airTemp   = NAN;
  r.waterTemp = NAN;
  return r;
}

// ---------------------------------------------------------------------------
// Small append helper: writes into buf at *pos, never past size.
// Returns false once the buffer is too small.
// ---------------------------------------------------------------------------
inline bool jsonAppend(char* buf, size_t size, size_t* pos, const char* fmt, ...) {
  if (*pos >= size) return false;
  va_list args;
  va_start(args, fmt);
  int n = vsnprintf(buf + *pos, size - *pos, fmt, args);
  va_end(args);
  if (n < 0 || (size_t)n >= size - *pos) return false;
  *pos += (size_t)n;
  return true;
}

// "key": <number with N decimals>   or   "key": null
inline bool jsonAppendNumber(char* buf, size_t size, size_t* pos,
                             const char* key, float value, int decimals) {
  if (isnan(value) || isinf(value)) {
    return jsonAppend(buf, size, pos, ",\"%s\":null", key);
  }
  return jsonAppend(buf, size, pos, ",\"%s\":%.*f", key, decimals, (double)value);
}

// ---------------------------------------------------------------------------
// Build the JSON body for one reading, e.g.
//   {"Time":"2026-09-23T16:14:00","pH":null,"Biomass":null,"AirTemp":null,"WaterTemp":null}
// Returns the length written, or -1 if buf is too small (buf is then "").
// ---------------------------------------------------------------------------
inline int buildReadingJson(const Reading& r, char* buf, size_t size) {
  if (size == 0) return -1;
  size_t pos = 0;
  bool ok;

  if (r.hasTime) {
    ok = jsonAppend(buf, size, &pos,
                    "{\"%s\":\"%04d-%02d-%02dT%02d:%02d:%02d%s\"",
                    KEY_TIME, r.year, r.month, r.day,
                    r.hour, r.minute, r.second, TIME_UTC_OFFSET);
  } else {
    ok = jsonAppend(buf, size, &pos, "{\"%s\":null", KEY_TIME);
  }

  ok = ok && jsonAppendNumber(buf, size, &pos, KEY_PH,         r.ph,        DECIMALS_PH);
  ok = ok && jsonAppendNumber(buf, size, &pos, KEY_BIOMASS,    r.biomass,   DECIMALS_BIOMASS);
  ok = ok && jsonAppendNumber(buf, size, &pos, KEY_AIR_TEMP,   r.airTemp,   DECIMALS_AIR_TEMP);
  ok = ok && jsonAppendNumber(buf, size, &pos, KEY_WATER_TEMP, r.waterTemp, DECIMALS_WATER_TEMP);
  ok = ok && jsonAppend(buf, size, &pos, "}");

  if (!ok) { buf[0] = '\0'; return -1; }
  return (int)pos;
}

#endif  // TELEMETRY_READING_H
