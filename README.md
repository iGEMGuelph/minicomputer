# Cultivator — Grow-Light Controller

**iGEM Guelph 2026**

Firmware for the Lumifert: a controlled-environment device. This repository holds the code that runs on the device's
microcontroller.

## Quick start

In short:
1. Wire the hardware (Section 3 of the build guide).
2. Install the Arduino IDE + RTClib library (Section 5).
3. Open `firmware/cultivator/cultivator.ino` and upload it (Section 5).
4. Open the Serial Monitor at 115200 and type `help`.



## API (for the dashboard)

Once the board is on WiFi it runs a small web server (port 80). The dashboard's
backend calls it to check whether the cultivator is online.

| Endpoint | Method | Handler | Reply | Notes |
|----------|--------|---------|-------|-------|
| `/api/status` | `GET` | `src/api/api.cpp` `serveClient()` | `{"device":"cultivator"}` | Any other path or method returns 404. |

- **Authentication:** every request must send the header
  `X-Device-Key: <DEVICE_SHARED_KEY>`. Requests with a missing or wrong key get
  `401 Unauthorized` before any routing. The key is `#define DEVICE_SHARED_KEY`
  in `secrets.h` (the build stops with an error if it's missing) and must match
  `DEVICE_SHARED_KEY` in the dashboard backend.
- **Finding the address:** once WiFi connects, the Serial Monitor prints
  `API listening: http://<ip>/api/status`. Set that address as `MINICOMPUTER_URL`
  in the dashboard repo's `backend/config.py`.
- **Status:** compiles, but not yet run on a real board.


## License

This project is licensed under the Apache License 2.0.
