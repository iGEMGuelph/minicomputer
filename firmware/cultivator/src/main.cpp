/* ===========================================================================
   main.cpp  —  combined entry point
   ---------------------------------------------------------------------------
   Runs three modules side by side on one board:
     * cultivator  (src/cultivator/) — grow-light schedule/dimming, RTC, Serial menu
     * sign-on     (src/sign-on/)    — private WiFi, falling back to eduroam
     * remote      (src/remote/)     — polls the dashboard for dimmer levels

   Each module exposes an xxxSetup()/xxxLoop() pair; this is the only place the
   Arduino setup()/loop() are defined. The lights are configured and forced OFF
   before anything touches the network.
   =========================================================================== */

#include <Arduino.h>

void cultivatorSetup();
void cultivatorLoop();

void signOnSetup();
void signOnLoop();

void remoteSetup();
void remoteLoop();

void setup() {
  Serial.begin(115200);
  delay(300);

  cultivatorSetup();
  signOnSetup();
  remoteSetup();
}

void loop() {
  cultivatorLoop();
  signOnLoop();
  remoteLoop();
}
