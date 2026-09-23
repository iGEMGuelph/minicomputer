/*
  sensors.h  —  THE PLACE TO PLUG IN REAL SENSORS

  Each function returns one measurement, or NAN when there is no reading.
  NAN is sent to the backend as JSON null, which the dashboard shows as
  "no data" instead of a fake number.

  Right now NONE of these sensors exist on the board, so every function
  returns NAN. When hardware adds a sensor:
    1. Wire it and add its pin(s) below.
    2. Initialise it in sensorsSetup() in sensors.cpp.
    3. Replace "return NAN;" in its read function with the real read.
    4. Return NAN if the read fails — never a made-up fallback value.
  Nothing else in the telemetry code needs to change.

  These are called from the main loop (not the upload task), so it is safe to
  use Wire / analogRead / OneWire here like anywhere else in the sketch.
  Keep each read quick (well under a second).
*/

#ifndef TELEMETRY_SENSORS_H
#define TELEMETRY_SENSORS_H

// Pins — fill these in once the sensors are wired (-1 = not connected).
#define PH_SENSOR_PIN          -1
#define BIOMASS_SENSOR_PIN     -1
#define AIR_TEMP_SENSOR_PIN    -1
#define WATER_TEMP_SENSOR_PIN  -1

void  sensorsSetup();

float readPH();          // acidity, 0.0 - 14.0
float readBiomass();     // duckweed coverage as a proportion, 0.00 - 1.00
float readAirTemp();     // degrees Celsius
float readWaterTemp();   // degrees Celsius

#endif  // TELEMETRY_SENSORS_H
