/*
  cultivator.h  —  the grow-light module's public interface.
  Other modules (api, backend) read and drive the lights through these;
  everything else in cultivator.cpp is internal.
*/

#ifndef CULTIVATOR_H
#define CULTIVATOR_H

#include <RTClib.h>

/**
 *   MODE_AUTO   - follow the RTC daily schedule (power-on default)
 *   MODE_MANUAL - hold levels typed at the serial menu; the dashboard is ignored
 *   MODE_REMOTE - follow the dashboard's dimmer sliders (src/backend/)
 */
enum Mode { MODE_AUTO, MODE_MANUAL, MODE_REMOTE };

Mode currentMode();
/** Lowercase name sent to the backend: "auto", "manual" or "remote". */
const char* modeName(Mode m);
/** Switch modes and apply the new mode's levels immediately. */
void setMode(Mode m);

/** Brightness (0-100) currently being driven on each colour. */
int lightRed();
int lightBlue();

/** Store the dashboard's levels (clamped 0-100); applied now if in MODE_REMOTE. */
void setRemoteLevels(int redPct, int bluePct);

bool rtcPresent();
/** Current RTC time. Returns false (and leaves `out` alone) if there is no RTC. */
bool cultivatorNow(DateTime& out);

void cultivatorSetup();
void cultivatorLoop();

#endif  // CULTIVATOR_H
