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

// ===========================================================================
// Kick off a connection attempt on the given network.
//   NET_PRIVATE -> plain WPA2-PSK  (NOAH_IDENTITY / NOAH_PASSWORD)
//   NET_EDUROAM -> WPA2-Enterprise (PEAP, EAP_IDENTITY / EAP_USERNAME / EAP_PASSWORD)
// ===========================================================================
void startConnect(NetworkMode mode) {
  currentNetwork = mode;

  WiFi.disconnect(true, true);   // clear any stored/prior WiFi state
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);          // enterprise auth is flaky with modem sleep on

  if (mode == NET_PRIVATE) {
    Serial.println(F("Starting private WiFi sign-on..."));
    Serial.print  (F("  SSID: ")); Serial.println(NOAH_IDENTITY);
    WiFi.begin(NOAH_IDENTITY, NOAH_PASSWORD);
  } else {
    Serial.println(F("Starting eduroam sign-on..."));
    Serial.print  (F("  identity: ")); Serial.println(EAP_IDENTITY);
    Serial.print  (F("  username: ")); Serial.println(EAP_USERNAME);
    WiFi.begin(EDUROAM_SSID, WPA2_AUTH_PEAP, EAP_IDENTITY, EAP_USERNAME, EAP_PASSWORD);
  }

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
