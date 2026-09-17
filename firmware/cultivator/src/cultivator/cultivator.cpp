/* ===========================================================================
   Cultivator  —  iGEM Guelph 2026
   Grow-light controller firmware                                  v0.3.0
   ---------------------------------------------------------------------------
   Matches the Prototype Schematic V1.0 (updated 2026-07-08):
   Arduino Nano ESP32 + 2 LED drivers (red/blue) + 2 low-side MOSFETs + DS3231 RTC.

   WHAT THIS DOES:
     * Dims four colour channels independently (PWM through MOSFET gates).
       Only red and blue are wired on the V1.0 board; see config.h.
     * Takes levels from the RTC daily schedule (AUTO), the dashboard over WiFi
       (REMOTE, see ../remote/), or the Serial menu (MANUAL).

   WHAT THIS DOES NOT DO (hardware isn't there — see docs/ROADMAP.md):
     * No temperature cutoff, no sensors.

   SAFETY NOTES (why this cannot harm the hardware):
     * The LED drivers current-limit each LED to a safe 700 mA no matter what
       the code does, so the LEDs cannot be over-driven from software.
     * The 10k pulldown on each MOSFET gate keeps both lights OFF during
       power-up and while the code is being uploaded.
     * On boot, the code explicitly sets every light to OFF before anything else.

   BOARD:      Arduino Nano ESP32
   LIBRARIES:  RTClib by Adafruit, ArduinoJson (see platformio.ini)
   SETTINGS:   edit config.h (pins, brightness, schedule).
   =========================================================================== */

#include <Arduino.h>
#include "config.h"
#include "cultivator.h"
#include "schedule.h"
#include <Wire.h>
#include <RTClib.h>

static const int   PWM_MAX  = (1 << PWM_RES_BITS) - 1;   // 8 bits -> 255
static const char* VERSION  = "0.3.0";

// File-scope state is static so it cannot collide with the other modules'
// globals. Channel tables are indexed by the LightChannel enum.
static const int  CHANNEL_PIN[CH_COUNT]     = {
  UVB_LED_PIN, RED_LED_PIN, FAR_RED_LED_PIN, BLUE_LED_PIN
};
static const int  CHANNEL_DAY_PCT[CH_COUNT] = {
  UVB_DAY_PCT, RED_DAY_PCT, FAR_RED_DAY_PCT, BLUE_DAY_PCT
};
static const char* CHANNEL_NAME[CH_COUNT]   = { "uvb", "red", "far_red", "blue" };
// Human-friendly labels for the serial menu, same order.
static const char* CHANNEL_LABEL[CH_COUNT]  = { "UVB", "RED", "FAR RED", "BLUE" };

// arduino-esp32 2.x LEDC API (channel-based). The LightChannel index doubles as
// the LEDC channel number.
static RTC_DS3231 rtc;
static bool  rtcOk = false;           // true if the DS3231 clock was found

static Mode  mode = MODE_AUTO;        // AUTO = follow schedule (power-on default)

static int   manualLevels[CH_COUNT]  = {0, 0, 0, 0};  // levels used in MANUAL mode
static int   remoteLevels[CH_COUNT]  = {0, 0, 0, 0};  // levels pushed by the dashboard
static int   currentLevels[CH_COUNT] = {0, 0, 0, 0};  // levels actually applied

static int   schedOnMin  = SCHEDULE_ON_HOUR  * 60;   // schedule as minutes-into-day
static int   schedOffMin = SCHEDULE_OFF_HOUR * 60;

static bool  warnedNoRtc = false;
static unsigned long lastUpdate = 0;

// ===========================================================================
// LIGHT OUTPUT  —  set each colour's brightness (0-100%) via PWM duty cycle
// ===========================================================================
static void applyOutputs(const int levels[CH_COUNT]) {
  for (int ch = 0; ch < CH_COUNT; ch++) {
    int pct = constrain(levels[ch], 0, 100);
    if (CHANNEL_PIN[ch] != NOT_WIRED) {
      ledcWrite(ch, map(pct, 0, 100, 0, PWM_MAX));
    }
    currentLevels[ch] = pct;
  }
}

// Current time as minutes-since-midnight (0..1439) from the RTC.
static int nowMinutes() {
  DateTime n = rtc.now();
  return n.hour() * 60 + n.minute();
}

// ===========================================================================
// MAIN UPDATE  —  decide what the lights should be doing and apply it
// ===========================================================================
static void update() {
  if (mode == MODE_MANUAL) {
    applyOutputs(manualLevels);
    return;
  }
  if (mode == MODE_REMOTE) {
    applyOutputs(remoteLevels);
    return;
  }
  // AUTO: follow the daily schedule. Requires the RTC to know the time.
  if (!rtcOk) {
    if (!warnedNoRtc) {
      Serial.println(F("AUTO needs the RTC clock, which was not found."));
      Serial.println(F("Lights held OFF. Wire the RTC, or use manual commands (e.g. 'red 10')."));
      warnedNoRtc = true;
    }
    int off[CH_COUNT] = {0, 0, 0, 0};
    applyOutputs(off);
    return;
  }
  bool day = isDaytimeMinutes(nowMinutes(), schedOnMin, schedOffMin);
  int wanted[CH_COUNT];
  for (int ch = 0; ch < CH_COUNT; ch++) wanted[ch] = day ? CHANNEL_DAY_PCT[ch] : 0;
  applyOutputs(wanted);
}

// ===========================================================================
// PUBLIC INTERFACE  (declared in cultivator.h)
// ===========================================================================
const char* channelName(int channel) {
  if (channel < 0 || channel >= CH_COUNT) return "?";
  return CHANNEL_NAME[channel];
}

bool channelIsWired(int channel) {
  if (channel < 0 || channel >= CH_COUNT) return false;
  return CHANNEL_PIN[channel] != NOT_WIRED;
}

int channelLevel(int channel) {
  if (channel < 0 || channel >= CH_COUNT) return 0;
  return currentLevels[channel];
}

Mode currentMode() { return mode; }

bool rtcPresent() { return rtcOk; }

const char* modeName(Mode m) {
  switch (m) {
    case MODE_AUTO:   return "auto";
    case MODE_MANUAL: return "manual";
    case MODE_REMOTE: return "remote";
  }
  return "?";
}

void setMode(Mode m) {
  if (m == MODE_AUTO) warnedNoRtc = false;
  if (m == MODE_MANUAL) {
    // Hold whatever is lit right now, not a stale manual set.
    for (int ch = 0; ch < CH_COUNT; ch++) manualLevels[ch] = currentLevels[ch];
  }
  mode = m;
  update();
}

void setAllLevels(const int levels[CH_COUNT]) {
  for (int ch = 0; ch < CH_COUNT; ch++) {
    remoteLevels[ch] = constrain(levels[ch], 0, 100);
  }
  if (mode == MODE_REMOTE) update();
}

// ===========================================================================
// SERIAL COMMAND INTERFACE
// ===========================================================================
static void printBanner() {
  Serial.println();
  Serial.println(F("========================================"));
  Serial.print  (F("  Cultivator grow-light controller v")); Serial.println(VERSION);
  Serial.println(F("  iGEM Guelph 2026"));
  Serial.println(F("========================================"));
  Serial.print  (F("  Clock: "));
  Serial.println(rtcOk ? F("DS3231 found") : F("NOT found (schedule needs it)"));
  Serial.println(F("  Type 'help' for commands."));
  Serial.println();
}

static void printHelp() {
  Serial.println(F("Commands (type one, then Enter):"));
  Serial.println(F("  help                 show this list"));
  Serial.println(F("  status               show current state"));
  Serial.println(F("  uvb <0-100>          set UVB brightness     (switches to manual)"));
  Serial.println(F("  red <0-100>          set RED brightness     (switches to manual)"));
  Serial.println(F("  farred <0-100>       set FAR RED brightness (switches to manual)"));
  Serial.println(F("  blue <0-100>         set BLUE brightness    (switches to manual)"));
  Serial.println(F("  all <0-100>          set every colour"));
  Serial.println(F("  off                  all colours off (manual)"));
  Serial.println(F("  auto                 resume the daily schedule"));
  Serial.println(F("  manual               hold current brightness, ignore schedule"));
  Serial.println(F("  remote               follow the dashboard over WiFi"));
  Serial.println(F("  schedule <on> <off>  set ON and OFF hours, 0-23"));
  Serial.println(F("  settime Y M D h m s  set the clock, e.g. settime 2026 7 9 14 30 0"));
  Serial.println(F("  gettime              show the clock time"));
}

static void printStatus() {
  Serial.println(F("---- STATUS ----"));
  Serial.print(F("  mode      : ")); Serial.println(modeName(mode));
  Serial.print(F("  schedule  : ON ")); Serial.print(schedOnMin / 60);
  Serial.print(F(":00  OFF ")); Serial.print(schedOffMin / 60); Serial.println(F(":00"));
  if (rtcOk) {
    int nm = nowMinutes();
    Serial.print(F("  time (RTC): ")); Serial.print(nm / 60); Serial.print(':');
    if (nm % 60 < 10) Serial.print('0');
    Serial.println(nm % 60);
  } else {
    Serial.println(F("  time (RTC): clock NOT found"));
  }
  for (int ch = 0; ch < CH_COUNT; ch++) {
    Serial.print(F("  "));
    Serial.print(CHANNEL_LABEL[ch]);
    for (int pad = strlen(CHANNEL_LABEL[ch]); pad < 10; pad++) Serial.print(' ');
    Serial.print(F(": ")); Serial.print(currentLevels[ch]); Serial.print('%');
    if (!channelIsWired(ch)) Serial.print(F("   (NOT WIRED — no driver on this board)"));
    Serial.println();
  }
  Serial.println(F("----------------"));
}

// Split a line into space-separated tokens.
static int tokenize(String line, String out[], int maxTokens) {
  int n = 0, start = 0;
  line.trim();
  while (n < maxTokens && start < (int)line.length()) {
    int sp = line.indexOf(' ', start);
    if (sp < 0) { out[n++] = line.substring(start); break; }
    if (sp > start) out[n++] = line.substring(start, sp);
    start = sp + 1;
    while (start < (int)line.length() && line.charAt(start) == ' ') start++;
  }
  return n;
}

// Map a typed command word to a channel, or -1 if it is not a colour command.
static int channelForCommand(const String& cmd) {
  if (cmd == "uvb")    return CH_UVB;
  if (cmd == "red")    return CH_RED;
  if (cmd == "farred") return CH_FAR_RED;
  if (cmd == "blue")   return CH_BLUE;
  return -1;
}

static void processCommand(String line) {
  String tok[8];
  int n = tokenize(line, tok, 8);
  if (n == 0) return;
  String cmd = tok[0];
  cmd.toLowerCase();

  int ch = channelForCommand(cmd);
  if (ch >= 0 && n >= 2) {
    // A colour command takes local control; the person at the console wins.
    if (mode != MODE_MANUAL) setMode(MODE_MANUAL);
    manualLevels[ch] = constrain(tok[1].toInt(), 0, 100);
    update();
    Serial.print(CHANNEL_LABEL[ch]);
    Serial.print(F(" set to ")); Serial.print(currentLevels[ch]);
    Serial.print(F("% (manual)"));
    if (!channelIsWired(ch)) Serial.print(F("  — NOTE: this channel is NOT WIRED"));
    Serial.println();
    return;
  }

  if (cmd == "help") {
    printHelp();
  } else if (cmd == "status") {
    printStatus();
  } else if (cmd == "all" && n >= 2) {
    if (mode != MODE_MANUAL) setMode(MODE_MANUAL);
    int v = constrain(tok[1].toInt(), 0, 100);
    for (int i = 0; i < CH_COUNT; i++) manualLevels[i] = v;
    update();
    Serial.print(F("ALL set to ")); Serial.print(v); Serial.println(F("% (manual)"));
  } else if (cmd == "off") {
    if (mode != MODE_MANUAL) setMode(MODE_MANUAL);
    for (int i = 0; i < CH_COUNT; i++) manualLevels[i] = 0;
    update();
    Serial.println(F("All colours OFF (manual)"));
  } else if (cmd == "auto") {
    setMode(MODE_AUTO);
    Serial.println(F("AUTO mode - following the daily schedule"));
  } else if (cmd == "manual") {
    setMode(MODE_MANUAL);
    Serial.println(F("MANUAL mode - holding current brightness"));
  } else if (cmd == "remote") {
    setMode(MODE_REMOTE);
    Serial.println(F("REMOTE mode - following the dashboard"));
  } else if (cmd == "schedule" && n >= 3) {
    int on  = constrain(tok[1].toInt(), 0, 23);
    int off = constrain(tok[2].toInt(), 0, 23);
    schedOnMin = on * 60; schedOffMin = off * 60;
    Serial.print(F("Schedule set: ON ")); Serial.print(on);
    Serial.print(F(":00  OFF ")); Serial.print(off); Serial.println(F(":00"));
  } else if (cmd == "settime" && n >= 7) {
    if (rtcOk) {
      rtc.adjust(DateTime(tok[1].toInt(), tok[2].toInt(), tok[3].toInt(),
                          tok[4].toInt(), tok[5].toInt(), tok[6].toInt()));
      Serial.println(F("Clock updated."));
    } else {
      Serial.println(F("No RTC found - cannot set clock."));
    }
  } else if (cmd == "gettime") {
    if (rtcOk) {
      DateTime n2 = rtc.now();
      char buf[25];
      snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
               n2.year(), n2.month(), n2.day(), n2.hour(), n2.minute(), n2.second());
      Serial.println(buf);
    } else {
      Serial.println(F("No RTC found."));
    }
  } else {
    Serial.print(F("Unknown command: ")); Serial.println(line);
    Serial.println(F("Type 'help' for the list."));
  }
}

// Read typed input one line at a time (non-blocking).
static void handleSerial() {
  static String buf;
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (buf.length() > 0) { processCommand(buf); buf = ""; }
    } else {
      buf += c;
      if (buf.length() > 100) buf = "";   // overflow guard
    }
  }
}

// ===========================================================================
// SETUP  &  LOOP
// ===========================================================================
void cultivatorSetup() {
  // PWM on every wired channel, then force the lights OFF (safe default).
  for (int ch = 0; ch < CH_COUNT; ch++) {
    if (CHANNEL_PIN[ch] == NOT_WIRED) continue;
    ledcSetup(ch, PWM_FREQ, PWM_RES_BITS);
    ledcAttachPin(CHANNEL_PIN[ch], ch);
  }
  int off[CH_COUNT] = {0, 0, 0, 0};
  applyOutputs(off);

  // Find the clock. If missing, AUTO holds the lights off and warns.
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  rtcOk = rtc.begin();
  if (rtcOk && rtc.lostPower()) {
    // Clock lost its time (dead battery / first use): seed with compile time.
    // Use the "settime" command to set it precisely.
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  printBanner();
}

void cultivatorLoop() {
  handleSerial();
  if (millis() - lastUpdate >= UPDATE_INTERVAL_MS) {
    lastUpdate = millis();
    update();
  }
}
