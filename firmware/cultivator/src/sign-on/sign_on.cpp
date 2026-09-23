#include <Arduino.h>
#include "config.h"
#include "secrets.h"
#include <WiFi.h>

enum LinkState { LINK_CONNECTING, LINK_AUTH_FAILED, LINK_AUTHENTICATED, LINK_VERIFIED };
LinkState linkState = LINK_CONNECTING;

// Which network we're currently trying/connected to. We always try the
// private network first and only fall back to eduroam if it times out.
enum NetworkMode { NET_PRIVATE, NET_EDUROAM };
NetworkMode currentNetwork = NET_PRIVATE;

unsigned long connectStartedAt = 0;
unsigned long lastStatusPrint  = 0;

void printAuthResult(bool authenticated);   // defined below, used by startConnect's eduroam stub

// ===========================================================================
// Kick off a connection attempt on the given network.
//   NET_PRIVATE -> plain WPA2-PSK  (NOAH_IDENTITY / NOAH_PASSWORD)
//   NET_EDUROAM -> WPA2-Enterprise (PEAP, EAP_IDENTITY / EAP_USERNAME / EAP_PASSWORD)
//
// NOTE: eduroam is currently a stub on this board. The Arduino GIGA R1 WiFi's
// library ships a WiFi.beginEnterprise(), but Arduino's own core maintainers
// document it as incomplete for real two-phase WPA2-Enterprise networks like
// eduroam (unlike the ESP32 core this firmware previously targeted, which had
// full esp_wpa2.h PEAP support). Rather than attempt a connection that's known
// not to work, we keep the NET_PRIVATE -> NET_EDUROAM fallback shape intact
// (for when GIGA/mbed enterprise support matures) but fail eduroam immediately.
// ===========================================================================
void startConnect(NetworkMode mode) {
  currentNetwork = mode;

  WiFi.disconnect();   // clear any prior WiFi state

  if (mode == NET_PRIVATE) {
    Serial.println(F("Starting private WiFi sign-on..."));
    Serial.print  (F("  SSID: ")); Serial.println(NOAH_IDENTITY);
    WiFi.begin(NOAH_IDENTITY, NOAH_PASSWORD);
    connectStartedAt = millis();
    linkState = LINK_CONNECTING;
  } else {
    Serial.println(F("Skipping eduroam: WPA2-Enterprise is not reliably"));
    Serial.println(F("supported by the GIGA R1's WiFi library (stub only)."));
    connectStartedAt = millis();
    linkState = LINK_AUTH_FAILED;
    printAuthResult(false);
  }
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
  const char* label = (currentNetwork == NET_PRIVATE) ? "private WiFi" : "eduroam";
  Serial.println(F("---------------------------------------------"));
  if (authenticated) {
    Serial.print  (F("RESULT: AUTHENTICATED on ")); Serial.println(label);
    Serial.print  (F("  IP address : ")); Serial.println(WiFi.localIP());
    Serial.print  (F("  RSSI       : ")); Serial.print(WiFi.RSSI()); Serial.println(F(" dBm"));
  } else {
    Serial.print  (F("RESULT: Sign-on to ")); Serial.print(label); Serial.println(F(" FAILED."));
    if (currentNetwork == NET_PRIVATE) {
      Serial.println(F("  Check: NOAH_IDENTITY (SSID) / NOAH_PASSWORD correct? in range?"));
    } else {
      Serial.println(F("  Check: passkey correct? identity/username correct?"));
      Serial.println(F("  Some schools need EAP_IDENTITY = \"anonymous@...\","));
      Serial.println(F("  others need it to match EAP_USERNAME exactly."));
    }
  }
  Serial.println(F("---------------------------------------------"));
}

// ===========================================================================
// SETUP  &  LOOP
// ===========================================================================
void signOnSetup() {
  Serial.println();
  Serial.println(F("==== WiFi sign-on (private, falls back to eduroam) ===="));
  startConnect(NET_PRIVATE);
}

void signOnLoop() {
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
        if (currentNetwork == NET_PRIVATE) {
          Serial.println(F("Private WiFi timed out — falling back to eduroam..."));
          startConnect(NET_EDUROAM);
        } else {
          linkState = LINK_AUTH_FAILED;
          printAuthResult(false);
        }
      }
      break;
    }

    case LINK_AUTH_FAILED:
      // Stay failed; retry automatically after a cooldown, starting again
      // from the private network.
      if (millis() - connectStartedAt > WIFI_CONNECT_TIMEOUT_MS * 3) {
        startConnect(NET_PRIVATE);
      }
      break;

    case LINK_AUTHENTICATED:
    case LINK_VERIFIED:
      if (WiFi.status() != WL_CONNECTED) {
        Serial.println(F("Link dropped. Reconnecting..."));
        startConnect(NET_PRIVATE);
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
