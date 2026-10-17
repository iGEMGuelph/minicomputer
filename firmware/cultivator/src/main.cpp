/* ===========================================================================
   main.cpp  —  combined entry point
   ---------------------------------------------------------------------------
   Runs these modules side by side on one board:
     * cultivator  (src/cultivator/) — grow-light schedule/dimming, RTC, Serial menu
     * sign-on     (src/sign-on/)    — eduroam WPA2-Enterprise connect + auth test
     * api         (src/api/)        — web server for the dashboard (GET /api/status)
     * backend     (src/backend/)    — outbound calls to the dashboard backend:
                                       light levels, light-state reports, readings

   Each module keeps its own setup/loop pair (cultivatorSetup/cultivatorLoop,
   signOnSetup/signOnLoop, apiSetup/apiLoop, backendSetup/backendLoop) instead of the Arduino setup()/loop() — this file is
   the only place setup()/loop() are defined, and it just calls them all in turn.
   Only backend blocks noticeably: each outbound request can hold loop() for up
   to HTTP_TIMEOUT_MS (src/backend/config.h), and it backs off to one attempt
   every BACKEND_RETRY_INTERVAL_MS while the backend is unreachable.
   =========================================================================== */

#include <Arduino.h>

void cultivatorSetup();
void cultivatorLoop();

void signOnSetup();
void signOnLoop();

void apiSetup();
void apiLoop();

void backendSetup();
void backendLoop();

void setup() {
  Serial.begin(115200);
  delay(300);

  cultivatorSetup();
  signOnSetup();
  apiSetup();
  backendSetup();
}

void loop() {
  cultivatorLoop();
  signOnLoop();
  apiLoop();
  backendLoop();
}
