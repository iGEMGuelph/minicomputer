/*
  config.h  —  THE ONE FILE YOU EDIT TO CHANGE SETTINGS

  Edit a number after a "#define", save, then re-upload to the board.
  (Network settings for the dashboard live separately, in ../remote/config.h.)
*/

#ifndef CONFIG_H
#define CONFIG_H

// ===========================================================================
// 1) WIRING  —  which Arduino pin connects to what
// ===========================================================================
// Use the pin LABELS printed on the Arduino Nano ESP32 board. The code is the
// single source of truth for pins; if the team wires differently, change the
// numbers here and nothing else.
//
// NOT_WIRED marks a colour with no LED driver on the board yet. It still accepts
// and reports a brightness (so the dashboard stays honest) but drives nothing.
// To add a colour later: fit the driver + MOSFET, then put its pin here.

#define NOT_WIRED      -1

#define UVB_LED_PIN     NOT_WIRED  // not on the V1.0 board
#define RED_LED_PIN     D3         // -> RED  MOSFET gate (through its 220 ohm resistor)
#define FAR_RED_LED_PIN NOT_WIRED  // not on the V1.0 board
#define BLUE_LED_PIN    D2         // -> BLUE MOSFET gate (through its 220 ohm resistor)

#define I2C_SDA_PIN    A4      // -> RTC clock  SDA
#define I2C_SCL_PIN    A5      // -> RTC clock  SCL
// RTC also needs: VCC -> 3V3,  GND -> GND (common ground).

// ===========================================================================
// 2) DAYTIME BRIGHTNESS  —  used in AUTO mode during the ON window (0-100)
// ===========================================================================
#define UVB_DAY_PCT      0
#define RED_DAY_PCT     80
#define FAR_RED_DAY_PCT  0
#define BLUE_DAY_PCT    60

// ===========================================================================
// 3) DAILY SCHEDULE  —  24-hour clock. Wrapping past midnight is allowed.
// ===========================================================================
#define SCHEDULE_ON_HOUR    6      // hour lights turn ON   (0-23)
#define SCHEDULE_OFF_HOUR   22     // hour lights turn OFF  (0-23)

// ===========================================================================
// 4) ADVANCED  —  rarely changed
// ===========================================================================
#define PWM_FREQ            5000   // dimming frequency in Hz (flicker-free, silent)
#define PWM_RES_BITS        8      // 8-bit dimming = 256 brightness steps
#define UPDATE_INTERVAL_MS  1000   // how often the schedule is re-checked (ms)

#endif  // CONFIG_H
