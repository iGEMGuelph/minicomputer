/*
  cultivator.h  —  the grow-light module's public interface.
  Other modules drive the lights through these; everything else in
  cultivator.cpp is static.
*/

#ifndef CULTIVATOR_H
#define CULTIVATOR_H

/** Light channels, in the order the API lists them. */
enum LightChannel { CH_UVB = 0, CH_RED, CH_FAR_RED, CH_BLUE, CH_COUNT };

/**
 *   MODE_AUTO   - follow the RTC daily schedule (power-on default)
 *   MODE_MANUAL - hold levels typed at the serial menu; the dashboard is ignored
 *   MODE_REMOTE - follow the dashboard
 */
enum Mode { MODE_AUTO, MODE_MANUAL, MODE_REMOTE };

/** Lowercase API name of a channel: "uvb", "red", "far_red", "blue". */
const char* channelName(int channel);
bool channelIsWired(int channel);
/** Brightness (0-100) currently being driven. */
int  channelLevel(int channel);

Mode currentMode();
const char* modeName(Mode m);
bool rtcPresent();

/** Switch modes and apply the new mode's levels immediately. */
void setMode(Mode m);

/** Set the dashboard's levels for all channels (clamped 0-100). Does not change the mode. */
void setAllLevels(const int levels[CH_COUNT]);

void cultivatorSetup();
void cultivatorLoop();

#endif  // CULTIVATOR_H
