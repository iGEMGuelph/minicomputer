/*
  schedule.h  —  Pure scheduling logic (NO Arduino dependencies)

  This is kept separate on purpose: because it uses only plain integers, it can
  be compiled and unit-tested on a normal computer (see tests/test_schedule.cpp),
  which lets us verify the tricky time math without real hardware.

  All times are "minutes since midnight" (0..1439). e.g. 06:00 = 360, 22:00 = 1320.
*/

#ifndef SCHEDULE_H
#define SCHEDULE_H

// Returns true if 'nowMin' falls inside the ON window [onMin, offMin).
// Correctly handles a window that wraps past midnight (onMin > offMin),
// e.g. ON 20:00 (1200) .. OFF 06:00 (360).
inline bool isDaytimeMinutes(int nowMin, int onMin, int offMin) {
  if (onMin == offMin) return true;                       // ON == OFF -> always on
  if (onMin <  offMin) return (nowMin >= onMin && nowMin < offMin);
  return (nowMin >= onMin || nowMin < offMin);            // window wraps past midnight
}

#endif  // SCHEDULE_H
