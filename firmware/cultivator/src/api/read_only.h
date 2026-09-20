/*
  read_only.h  —  the device-wide READ_ONLY flag (defined in api.cpp).

  true  = keep reporting readings, but refuse anything that changes the device.
  For now it is only reported over the API; nothing enforces it yet.
  Not saved: it resets to READ_ONLY_DEFAULT (false) whenever the board restarts.
*/

#ifndef API_READ_ONLY_H
#define API_READ_ONLY_H

extern bool READ_ONLY;

#endif  // API_READ_ONLY_H
