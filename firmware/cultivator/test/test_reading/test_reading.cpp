/*
  Unit tests for src/telemetry/reading.h (the JSON the board sends).
  Run on your computer, no board needed:   pio test -e native
*/

#include <unity.h>
#include <string.h>
#include "../../src/telemetry/reading.h"

static Reading timedReading() {
  Reading r = emptyReading();
  r.hasTime = true;
  r.year = 2026; r.month = 9; r.day = 3;
  r.hour = 7;    r.minute = 5; r.second = 9;
  return r;
}

void setUp() {}
void tearDown() {}

// Today's reality: only the clock exists, every sensor is null.
void test_time_only_sends_nulls() {
  Reading r = timedReading();
  char buf[TELEMETRY_JSON_BUF_SIZE];
  int len = buildReadingJson(r, buf, sizeof(buf));
  TEST_ASSERT_EQUAL_STRING(
    "{\"Time\":\"2026-09-03T07:05:09\",\"pH\":null,\"Biomass\":null,"
    "\"AirTemp\":null,\"WaterTemp\":null}", buf);
  TEST_ASSERT_EQUAL_INT((int)strlen(buf), len);
}

// Once sensors exist: values are rounded to the configured decimals.
void test_values_use_configured_decimals() {
  Reading r = timedReading();
  r.ph = 7.24f; r.biomass = 0.356f; r.airTemp = 22.5f; r.waterTemp = -1.0f;
  char buf[TELEMETRY_JSON_BUF_SIZE];
  buildReadingJson(r, buf, sizeof(buf));
  TEST_ASSERT_EQUAL_STRING(
    "{\"Time\":\"2026-09-03T07:05:09\",\"pH\":7.2,\"Biomass\":0.36,"
    "\"AirTemp\":22.50,\"WaterTemp\":-1.00}", buf);
}

// A sensor that fails mid-run (NAN) or returns garbage (inf) becomes null,
// never "nan"/"inf" (which would be invalid JSON).
void test_nan_and_inf_become_null() {
  Reading r = timedReading();
  r.ph = 7.0f; r.biomass = NAN; r.airTemp = INFINITY; r.waterTemp = -INFINITY;
  char buf[TELEMETRY_JSON_BUF_SIZE];
  buildReadingJson(r, buf, sizeof(buf));
  TEST_ASSERT_NOT_NULL(strstr(buf, "\"pH\":7.0"));
  TEST_ASSERT_NOT_NULL(strstr(buf, "\"Biomass\":null"));
  TEST_ASSERT_NOT_NULL(strstr(buf, "\"AirTemp\":null"));
  TEST_ASSERT_NOT_NULL(strstr(buf, "\"WaterTemp\":null"));
  TEST_ASSERT_NULL(strstr(buf, "nan"));
  TEST_ASSERT_NULL(strstr(buf, "inf"));
}

void test_missing_clock_sends_null_time() {
  Reading r = emptyReading();
  char buf[TELEMETRY_JSON_BUF_SIZE];
  buildReadingJson(r, buf, sizeof(buf));
  TEST_ASSERT_EQUAL_STRING(
    "{\"Time\":null,\"pH\":null,\"Biomass\":null,"
    "\"AirTemp\":null,\"WaterTemp\":null}", buf);
}

// Too-small buffer: reports failure and leaves an empty string, never a
// half-written (invalid) JSON message.
void test_small_buffer_fails_cleanly() {
  Reading r = timedReading();
  char buf[20];
  TEST_ASSERT_EQUAL_INT(-1, buildReadingJson(r, buf, sizeof(buf)));
  TEST_ASSERT_EQUAL_STRING("", buf);
}

// The worst case (every value present, big numbers) still fits the buffer.
void test_worst_case_fits_buffer() {
  Reading r = timedReading();
  r.ph = -99999.9f; r.biomass = -99999.99f; r.airTemp = -99999.99f; r.waterTemp = -99999.99f;
  char buf[TELEMETRY_JSON_BUF_SIZE];
  TEST_ASSERT_GREATER_THAN(0, buildReadingJson(r, buf, sizeof(buf)));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_time_only_sends_nulls);
  RUN_TEST(test_values_use_configured_decimals);
  RUN_TEST(test_nan_and_inf_become_null);
  RUN_TEST(test_missing_clock_sends_null_time);
  RUN_TEST(test_small_buffer_fails_cleanly);
  RUN_TEST(test_worst_case_fits_buffer);
  return UNITY_END();
}
