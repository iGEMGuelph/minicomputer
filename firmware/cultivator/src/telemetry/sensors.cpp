/*
  sensors.cpp  —  sensor reads (all placeholders until hardware exists).
  See sensors.h for how to connect a real sensor.
*/

#include <Arduino.h>
#include <math.h>
#include "sensors.h"

void sensorsSetup() {
  // TODO(hardware): initialise sensors here, e.g. pinMode(), sensor.begin().
}

float readPH() {
  // TODO(hardware): no pH probe on the board yet.
  return NAN;
}

float readBiomass() {
  // TODO(hardware): no biomass sensor on the board yet.
  return NAN;
}

float readAirTemp() {
  // TODO(hardware): no air temperature sensor on the board yet.
  // (Do NOT use the DS3231's internal temperature — it measures the clock
  //  chip, not the air. See docs/ROADMAP.md.)
  return NAN;
}

float readWaterTemp() {
  // TODO(hardware): no water temperature probe on the board yet.
  return NAN;
}
