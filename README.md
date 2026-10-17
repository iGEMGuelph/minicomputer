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

Once the board is on WiFi it talks to the dashboard's backend in both
directions. Every request either way carries `X-Device-Key: <DEVICE_SHARED_KEY>`.

### Board → backend (outbound, `src/backend/`)

The board is the HTTP client here. Set the backend's address in
`src/backend/config.h` (`BACKEND_HOST`, `BACKEND_PORT`, `API_PREFIX`; the values
there are **placeholders**).

| Call | When | Body | Expected reply |
|------|------|------|----------------|
| `GET /api/getLightDims` | every 2 s | – | `200` `{"uvb","red","far_red","blue","revision"}` |
| `POST /api/reportLightState` | after each successful poll (doubles as the heartbeat) | `{"revision","mode","firmware","rtc_ok","rssi","applied":{…},"wired":{…}}` | `204` |
| `POST /api/logReading` | every 10 s (needs the RTC) | `{"Time":"2026-09-23T14:05:00","pH":null,"Biomass":null,"AirTemp":null,"WaterTemp":null}` | `201` |

- **Lights:** a successful poll switches AUTO → REMOTE, so the dashboard's red and
  blue sliders drive the lights. UVB and far-red aren't wired on this board; they
  are reported as `wired: false`, level 0. Typing a manual command at the serial
  menu (`red 10`, `manual`, …) takes over and the dashboard shows read-only until
  you type `remote` or `auto`. If the backend is unreachable the lights hold
  their levels, then fall back to the AUTO schedule after 5 minutes.
- **Readings:** `Time` is the RTC's local (Toronto) time with no offset. No
  sensors are wired yet, so all values are `null` (`src/backend/sensors.h`).
  Readings that fail to upload are kept (up to 30) and retried oldest first.
- **Logs:** every upload and any failure is printed on the Serial Monitor
  (`[backend] …`). Nothing else is sent anywhere.

### Backend → board (incoming, `src/api/`)

The board also runs a small web server (port 80) so the backend can check
whether it is online.

| Endpoint | Method | Handler | Reply | Notes |
|----------|--------|---------|-------|-------|
| `/api/status` | `GET` | `src/api/api.cpp` `serveClient()` | `{"device":"cultivator","readOnly":false}` | `readOnly` is `true` while the lights are in MANUAL mode. Any other path or method returns 404. |

- **Authentication:** every request must send the header
  `X-Device-Key: <DEVICE_SHARED_KEY>`. Requests with a missing or wrong key get
  `401 Unauthorized` before any routing. The key is `#define DEVICE_SHARED_KEY`
  in `secrets.h` (the build stops with an error if it's missing) and must match
  `DEVICE_SHARED_KEY` in the dashboard backend.
- **Finding the address:** once WiFi connects, the Serial Monitor prints
  `API listening: http://<ip>/api/status`. Set that address as `MINICOMPUTER_URL`
  in the dashboard repo's `backend/config.py`.
- **Status:** `src/api/` compiles but hasn't been run on a real board;
  `src/backend/` hasn't been compiled yet.


## License

This project is licensed under the Apache License 2.0.
