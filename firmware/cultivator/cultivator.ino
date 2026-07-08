/* ===========================================================================
   Cultivator  —  iGEM Guelph 2026
   Grow-light controller firmware                                  v0.2.0
   ---------------------------------------------------------------------------
   Matches the Prototype Schematic V1.0 (updated 2026-07-08):
   Arduino Nano ESP32 + 2 LED drivers (red/blue) + 2 low-side MOSFETs + DS3231 RTC.

   WHAT THIS DOES (exactly the confirmed "Must/Should" hardware stories):
     * Turns the RED and BLUE LED channels on/off on a daily schedule (RTC).
     * Dims each colour independently (PWM through the MOSFET gates).
     * A simple Serial menu (USB) to test and tune it by hand.

   WHAT THIS DOES NOT DO (on purpose — hardware isn't there / it's future work):
     * No temperature cutoff  (no temperature sensor exists on this board).
     * No WiFi / app upload    (software feature for later).
     * No sensors (pH/CO2/etc).
     See docs/ROADMAP.md.

   SAFETY NOTES (why this cannot harm the hardware):
     * The LED drivers current-limit each LED to a safe 700 mA no matter what
       the code does, so the LEDs cannot be over-driven from software.
     * The 10k pulldown on each MOSFET gate keeps both lights OFF during
       power-up and while the code is being uploaded.
     * On boot, the code explicitly sets both lights to OFF before anything else.

   BOARD:      Arduino Nano ESP32
   LIBRARIES:  RTClib by Adafruit  (Arduino IDE -> Library Manager)
   SETTINGS:   edit config.h (pins, brightness, schedule).
   =========================================================================== */

#include "config.h"
#include "schedule.h"
#include <Wire.h>
#include <RTClib.h>

static const int   PWM_MAX  = (1 << PWM_RES_BITS) - 1;   // 8 bits -> 255
static const char* VERSION  = "0.2.0";

RTC_DS3231 rtc;
bool  rtcOk = false;                 // true if the DS3231 clock was found

enum Mode { MODE_AUTO, MODE_MANUAL };
Mode  mode = MODE_AUTO;              // AUTO = follow schedule, MANUAL = hold fixed levels

int   manualRed  = 0;                // brightness used in MANUAL mode (%)
int   manualBlue = 0;

int   schedOnMin  = SCHEDULE_ON_HOUR  * 60;   // schedule as minutes-into-day
int   schedOffMin = SCHEDULE_OFF_HOUR * 60;

int   currentRed  = 0;               // brightness currently applied
int   currentBlue = 0;

bool  warnedNoRtc = false;
unsigned long lastUpdate = 0;

// ===========================================================================
// LIGHT OUTPUT  —  set each colour's brightness (0-100%) via PWM duty cycle
// ===========================================================================
void applyOutputs(int redPct, int bluePct) {
  redPct  = constrain(redPct,  0, 100);
  bluePct = constrain(bluePct, 0, 100);
  ledcWrite(RED_LED_PIN,  map(redPct,  0, 100, 0, PWM_MAX));
  ledcWrite(BLUE_LED_PIN, map(bluePct, 0, 100, 0, PWM_MAX));
  currentRed  = redPct;
  currentBlue = bluePct;
}

// Current time as minutes-since-midnight (0..1439) from the RTC.
int nowMinutes() {
  DateTime n = rtc.now();
  return n.hour() * 60 + n.minute();
}

// ===========================================================================
// MAIN UPDATE  —  decide what the lights should be doing and apply it
// ===========================================================================
void update() {
  if (mode == MODE_MANUAL) {
    applyOutputs(manualRed, manualBlue);
    return;
  }
  // AUTO: follow the daily schedule. Requires the RTC to know the time.
  if (!rtcOk) {
    if (!warnedNoRtc) {
      Serial.println(F("AUTO needs the RTC clock, which was not found."));
      Serial.println(F("Lights held OFF. Wire the RTC, or use manual commands (e.g. 'red 10')."));
      warnedNoRtc = true;
    }
    applyOutputs(0, 0);
    return;
  }
  bool day = isDaytimeMinutes(nowMinutes(), schedOnMin, schedOffMin);
  applyOutputs(day ? RED_DAY_PCT : 0, day ? BLUE_DAY_PCT : 0);
}

// ===========================================================================
// SERIAL COMMAND INTERFACE
// ===========================================================================
void printBanner() {
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

void printHelp() {
  Serial.println(F("Commands (type one, then Enter):"));
  Serial.println(F("  help                 show this list"));
  Serial.println(F("  status               show current state"));
  Serial.println(F("  red <0-100>          set RED brightness  (switches to manual)"));
  Serial.println(F("  blue <0-100>         set BLUE brightness (switches to manual)"));
  Serial.println(F("  both <0-100>         set both colours"));
  Serial.println(F("  off                  both colours off (manual)"));
  Serial.println(F("  auto                 resume the daily schedule"));
  Serial.println(F("  manual               hold current brightness, ignore schedule"));
  Serial.println(F("  schedule <on> <off>  set ON and OFF hours, 0-23"));
  Serial.println(F("  settime Y M D h m s  set the clock, e.g. settime 2026 7 9 14 30 0"));
  Serial.println(F("  gettime              show the clock time"));
}

void printStatus() {
  Serial.println(F("---- STATUS ----"));
  Serial.print(F("  mode      : ")); Serial.println(mode == MODE_AUTO ? F("AUTO (schedule)") : F("MANUAL"));
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
  Serial.print(F("  red       : ")); Serial.print(currentRed);  Serial.println('%');
  Serial.print(F("  blue      : ")); Serial.print(currentBlue); Serial.println('%');
  Serial.println(F("----------------"));
}

// Split a line into space-separated tokens.
int tokenize(String line, String out[], int maxTokens) {
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

void processCommand(String line) {
  String tok[8];
  int n = tokenize(line, tok, 8);
  if (n == 0) return;
  String cmd = tok[0];
  cmd.toLowerCase();

  if (cmd == "help") {
    printHelp();
  } else if (cmd == "status") {
    printStatus();
  } else if (cmd == "red" && n >= 2) {
    mode = MODE_MANUAL; manualRed = tok[1].toInt(); update();
    Serial.print(F("RED set to ")); Serial.print(currentRed); Serial.println(F("% (manual)"));
  } else if (cmd == "blue" && n >= 2) {
    mode = MODE_MANUAL; manualBlue = tok[1].toInt(); update();
    Serial.print(F("BLUE set to ")); Serial.print(currentBlue); Serial.println(F("% (manual)"));
  } else if (cmd == "both" && n >= 2) {
    mode = MODE_MANUAL; manualRed = manualBlue = tok[1].toInt(); update();
    Serial.print(F("BOTH set to ")); Serial.print(currentRed); Serial.println(F("% (manual)"));
  } else if (cmd == "off") {
    mode = MODE_MANUAL; manualRed = manualBlue = 0; update();
    Serial.println(F("Both colours OFF (manual)"));
  } else if (cmd == "auto") {
    mode = MODE_AUTO; warnedNoRtc = false; update();
    Serial.println(F("AUTO mode - following the daily schedule"));
  } else if (cmd == "manual") {
    mode = MODE_MANUAL; manualRed = currentRed; manualBlue = currentBlue;
    Serial.println(F("MANUAL mode - holding current brightness"));
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
void handleSerial() {
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
void setup() {
  Serial.begin(115200);
  delay(300);

  // Set up PWM dimming on both colour channels, then force lights OFF.
  // (Safe default; also matches the "off by default" pulldown resistors.)
  ledcAttach(RED_LED_PIN,  PWM_FREQ, PWM_RES_BITS);
  ledcAttach(BLUE_LED_PIN, PWM_FREQ, PWM_RES_BITS);
  applyOutputs(0, 0);

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

void loop() {
  handleSerial();
  if (millis() - lastUpdate >= UPDATE_INTERVAL_MS) {
    lastUpdate = millis();
    update();
  }
}
