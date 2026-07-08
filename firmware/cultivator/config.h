/*
  config.h  —  THE ONE FILE YOU EDIT TO CHANGE SETTINGS

  Edit a number after a "#define", save, then re-upload to the board.
  You should NOT need to edit cultivator.ino for normal use.
*/

#ifndef CONFIG_H
#define CONFIG_H

// ===========================================================================
// 1) WIRING  —  which Arduino pin connects to what
// ===========================================================================
// IMPORTANT: wire the hardware to THESE pins. They are the pins the code uses.
// (The draft schematic's exact pin routing is hard to read, so the code is the
//  single source of truth. If the team wires to different pins, just change the
//  numbers here to match — nothing else needs to change.)
//
// Use the pin LABELS printed on the Arduino Nano ESP32 board.

#define BLUE_LED_PIN   D2      // -> BLUE MOSFET gate (through its 220 ohm resistor)
#define RED_LED_PIN    D3      // -> RED  MOSFET gate (through its 220 ohm resistor)
#define I2C_SDA_PIN    A4      // -> RTC clock  SDA
#define I2C_SCL_PIN    A5      // -> RTC clock  SCL
// RTC also needs: VCC -> 3V3,  GND -> GND (common ground).

// ===========================================================================
// 2) DAYTIME BRIGHTNESS  —  brightness used automatically during the ON window
// ===========================================================================
// 0 = off, 100 = full.

#define RED_DAY_PCT     80     // red brightness during the day (%)
#define BLUE_DAY_PCT    60     // blue brightness during the day (%)

// ===========================================================================
// 3) DAILY SCHEDULE  —  24-hour clock
// ===========================================================================
// Example: ON at 06:00, OFF at 22:00 (16 h on / 8 h off).
// Wrapping past midnight is allowed, e.g. ON 20, OFF 6.

#define SCHEDULE_ON_HOUR    6      // hour lights turn ON   (0-23)
#define SCHEDULE_OFF_HOUR   22     // hour lights turn OFF  (0-23)

// ===========================================================================
// 4) ADVANCED  —  rarely changed
// ===========================================================================
#define PWM_FREQ            5000   // dimming frequency in Hz (flicker-free, silent)
#define PWM_RES_BITS        8      // 8-bit dimming = 256 brightness steps
#define UPDATE_INTERVAL_MS  1000   // how often the schedule is re-checked (ms)

#endif  // CONFIG_H
