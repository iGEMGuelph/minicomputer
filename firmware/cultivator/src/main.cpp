/* ===========================================================================
   main.cpp  —  combined entry point
   ---------------------------------------------------------------------------
   Runs both modules side by side on one board:
     * cultivator  (src/cultivator/) — grow-light schedule/dimming, RTC, Serial menu
     * sign-on     (src/sign-on/)    — eduroam WPA2-Enterprise connect + auth test

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

void setup() {
  Serial.begin(115200);
  delay(300);

  cultivatorSetup();
  signOnSetup();
}

void loop() {
  cultivatorLoop();
  signOnLoop();
}
