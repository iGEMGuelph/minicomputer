/* ===========================================================================
   main.cpp  —  combined entry point
   ---------------------------------------------------------------------------
   Runs these modules side by side on one board:
     * cultivator  (src/cultivator/) — grow-light schedule/dimming, RTC, Serial menu
     * sign-on     (src/sign-on/)    — eduroam WPA2-Enterprise connect + auth test
     * api         (src/api/)        — READ_ONLY flag + web server for the dashboard

   Each module keeps its own setup/loop pair (cultivatorSetup/cultivatorLoop,
   signOnSetup/signOnLoop, apiSetup/apiLoop) instead of the Arduino setup()/loop() — this file is
   the only place setup()/loop() are defined, and it just calls them all in turn.
   None of the modules blocks for long in its loop function, so they share the
   single-threaded loop() fine.
   =========================================================================== */

#include <Arduino.h>

void cultivatorSetup();
void cultivatorLoop();

void signOnSetup();
void signOnLoop();

void apiSetup();
void apiLoop();

void setup() {
  Serial.begin(115200);
  delay(300);

  cultivatorSetup();
  signOnSetup();
  apiSetup();
}

void loop() {
  cultivatorLoop();
  signOnLoop();
  apiLoop();
}
