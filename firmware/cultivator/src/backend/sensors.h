/*
  sensors.h  —  PLACEHOLDER sensor reads for /logReading.

  None of these sensors exist on the board yet, so every read returns NAN,
  which is sent to the backend as JSON null ("no reading"). We never invent a
  number. When a sensor is wired, replace its function body with the real read
  and set its pin here. Keep each read quick (well under a second): it runs on
  the main loop alongside the lights.
*/

#ifndef BACKEND_SENSORS_H
#define BACKEND_SENSORS_H

#include <math.h>

// Pins — fill these in once the sensors are wired (-1 = not connected).
#define PH_SENSOR_PIN          -1
#define BIOMASS_SENSOR_PIN     -1
#define AIR_TEMP_SENSOR_PIN    -1
#define WATER_TEMP_SENSOR_PIN  -1

inline float readPH()        { return NAN; }   // acidity, 0.0 - 14.0
inline float readBiomass()   { return NAN; }   // duckweed coverage, 0.00 - 1.00
inline float readAirTemp()   { return NAN; }   // degrees Celsius
inline float readWaterTemp() { return NAN; }   // degrees Celsius

#endif  // BACKEND_SENSORS_H
