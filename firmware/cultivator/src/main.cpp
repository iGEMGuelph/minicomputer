/* ===========================================================================
   main.cpp  —  combined entry point
   ---------------------------------------------------------------------------
   Runs the modules side by side on one board:
     * cultivator  (src/cultivator/) — grow-light schedule/dimming, RTC, Serial menu
     * sign-on     (src/sign-on/)    — eduroam WPA2-Enterprise connect + auth test
     * telemetry   (src/telemetry/)  — reading every 10 s, POSTed to the backend
                                       (uploads run in their own FreeRTOS task)

   Each module keeps its own setup/loop pair (cultivatorSetup/cultivatorLoop,
   signOnSetup/signOnLoop) instead of the Arduino setup()/loop() — this file is
   the only place setup()/loop() are defined, and it just calls both in turn.
   Neither module blocks for long in its loop function, so they share the
   single-threaded loop() fine.
   =========================================================================== */

#include <Arduino.h>

void cultivatorSetup();
void cultivatorLoop();

void signOnSetup();
void signOnLoop();

void telemetrySetup();
void telemetryLoop();

void setup() {
  Serial.begin(115200);
  delay(300);

  cultivatorSetup();
  signOnSetup();
  telemetrySetup();   // after cultivatorSetup(): needs the RTC
}

void loop() {
  cultivatorLoop();
  signOnLoop();
  telemetryLoop();
}
