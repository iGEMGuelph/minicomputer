/* ===========================================================================
   sign-on  —  eduroam WPA2-Enterprise connect + auth test          v0.1.0
   ---------------------------------------------------------------------------
   Connects the board to eduroam (WPA2-Enterprise / 802.1X, PEAP-MSCHAPv2)
   on boot using the credentials in secrets.h, then reports whether the
   Microsoft account actually authenticated and whether traffic is flowing.

   SETUP (do this before uploading):
     1. Copy secrets.h.example -> secrets.h in this same folder.
     2. Edit secrets.h with your real eduroam identity/username/passkey.
        (secrets.h is gitignored — it will not be committed.)
     3. Optionally edit config.h (SSID, timeouts, test host).

   WHAT THIS DOES NOT DO:
     * This is NOT browser-based Microsoft SSO/OAuth — the Arduino has no
       browser and can't do interactive MFA. It performs standard 802.1X
       PEAP login using your Microsoft-account username + passkey directly,
       the same credential exchange eduroam has always used underneath.
     * Does not yet talk to the interface/live database — this sketch only
       proves the network leg works. Point TEST_HOST in config.h at your
       API server once you have one to get a real end-to-end check.

   SECURITY NOTE:
     eduroam's RADIUS server presents a TLS certificate as part of PEAP.
     This sketch does not pin/validate that certificate (ca_pem = NULL
     below), which is common in examples but means a rogue "eduroam" AP
     could intercept the login. For real deployment, get your institution's
     RADIUS CA certificate and pass it as the 6th argument to WiFi.begin().

   BOARD:  Arduino Nano ESP32  (or any arduino-esp32 core board — the
           enterprise WiFi.begin() overload used here needs the ESP32 core,
           not AVR/SAMD boards).
   =========================================================================== */

#include "config.h"
#include "secrets.h"
#include <WiFi.h>

enum LinkState { LINK_CONNECTING, LINK_AUTH_FAILED, LINK_AUTHENTICATED, LINK_VERIFIED };
LinkState linkState = LINK_CONNECTING;

unsigned long connectStartedAt = 0;
unsigned long lastStatusPrint  = 0;

// ===========================================================================
// Kick off the WPA2-Enterprise (PEAP) connection attempt.
// ===========================================================================
void startConnect() {
  Serial.println(F("Starting eduroam sign-on..."));
  Serial.print  (F("  identity: ")); Serial.println(EAP_IDENTITY);
  Serial.print  (F("  username: ")); Serial.println(EAP_USERNAME);

  WiFi.disconnect(true, true);   // clear any stored/prior WiFi state
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);          // enterprise auth is flaky with modem sleep on

  WiFi.begin(EDUROAM_SSID, WPA2_AUTH_PEAP, EAP_IDENTITY, EAP_USERNAME, EAP_PASSWORD);

  connectStartedAt = millis();
  linkState = LINK_CONNECTING;
}

// ===========================================================================
// Real "are we actually online" check: open a plain TCP connection.
// Successful 802.1X auth doesn't guarantee traffic is routed/allowed.
// ===========================================================================
bool testConnectivity() {
  Serial.print(F("Testing connectivity to "));
  Serial.print(TEST_HOST); Serial.print(':'); Serial.println(TEST_PORT);

  WiFiClient client;
  bool ok = client.connect(TEST_HOST, TEST_PORT);
  client.stop();
  return ok;
}

void printAuthResult(bool authenticated) {
  Serial.println(F("---------------------------------------------"));
  if (authenticated) {
    Serial.println(F("RESULT: Microsoft account AUTHENTICATED on eduroam."));
    Serial.print  (F("  IP address : ")); Serial.println(WiFi.localIP());
    Serial.print  (F("  RSSI       : ")); Serial.print(WiFi.RSSI()); Serial.println(F(" dBm"));
  } else {
    Serial.println(F("RESULT: Sign-on FAILED — not authenticated."));
    Serial.println(F("  Check: passkey correct? identity/username correct?"));
    Serial.println(F("  Some schools need EAP_IDENTITY = \"anonymous@...\","));
    Serial.println(F("  others need it to match EAP_USERNAME exactly."));
  }
  Serial.println(F("---------------------------------------------"));
}

// ===========================================================================
// SETUP  &  LOOP
// ===========================================================================
void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.println(F("==== eduroam sign-on test ===="));
  startConnect();
}

void loop() {
  switch (linkState) {

    case LINK_CONNECTING: {
      wl_status_t status = WiFi.status();
      if (status == WL_CONNECTED) {
        linkState = LINK_AUTHENTICATED;
        printAuthResult(true);
        Serial.println(F("Running connectivity test..."));
        linkState = testConnectivity() ? LINK_VERIFIED : LINK_AUTHENTICATED;
        if (linkState == LINK_VERIFIED) {
          Serial.println(F("Connectivity test PASSED — traffic is flowing."));
        } else {
          Serial.println(F("Connectivity test FAILED — authenticated but no"));
          Serial.println(F("route to TEST_HOST (captive portal? blocked port?)."));
        }
        lastStatusPrint = millis();
      } else if (millis() - connectStartedAt > WIFI_CONNECT_TIMEOUT_MS) {
        linkState = LINK_AUTH_FAILED;
        printAuthResult(false);
      }
      break;
    }

    case LINK_AUTH_FAILED:
      // Stay failed; retry automatically after a cooldown.
      if (millis() - connectStartedAt > WIFI_CONNECT_TIMEOUT_MS * 3) {
        startConnect();
      }
      break;

    case LINK_AUTHENTICATED:
    case LINK_VERIFIED:
      if (WiFi.status() != WL_CONNECTED) {
        Serial.println(F("Link dropped. Reconnecting..."));
        startConnect();
        break;
      }
      if (millis() - lastStatusPrint >= STATUS_PRINT_INTERVAL_MS) {
        lastStatusPrint = millis();
        Serial.print(F("[ok] connected, IP "));
        Serial.print(WiFi.localIP());
        Serial.print(F(", RSSI "));
        Serial.print(WiFi.RSSI());
        Serial.println(F(" dBm"));
      }
      break;
  }
}
