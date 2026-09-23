# Cultivator — Grow-Light Controller

**iGEM Guelph 2026**

Firmware for the *Cultivator*: a controlled-environment device for growing
**duckweed** (a fast-growing aquatic plant used as sustainable protein/food/
fertilizer). This repository holds the code that runs on the device's
microcontroller.

> **What the code does today:** controls a two-colour (red + blue) LED grow
> light (brightness + a daily on/off schedule from a real-time clock), connects
> to WiFi, and every 10 s sends a reading (time + pH / biomass / air & water
> temperature) to the dashboard backend. The sensors don't exist yet, so those
> values are sent as `null` until hardware adds them. See
> [`docs/TELEMETRY.md`](docs/TELEMETRY.md). The larger
> product vision (sensors, app, dashboard) is tracked in
> [`docs/ROADMAP.md`](docs/ROADMAP.md).

---

## Quick start

👉 **New to this? Read [`docs/BUILD_GUIDE.md`](docs/BUILD_GUIDE.md).** It walks
you from a box of parts all the way to working, tested lights — no experience
assumed.

In short:
1. Wire the hardware (Section 3 of the build guide).
2. Install PlatformIO and fill in the WiFi login + backend address (Section 5).
3. `cd firmware/cultivator && pio run -t upload`
4. Open the Serial Monitor at 115200 and type `help`.

## Repository layout

```
minicomputer/
├── README.md                      <- you are here
├── firmware/cultivator/           PlatformIO project (the code on the board)
│   ├── platformio.ini             build settings; `pio run`, `pio test -e native`
│   ├── src/
│   │   ├── main.cpp               setup()/loop(): runs the three modules below
│   │   ├── cultivator/            grow lights + RTC + serial menu
│   │   │   ├── config.h           pins, brightness, schedule
│   │   │   └── schedule.h         on/off time logic
│   │   ├── sign-on/               WiFi (private network, eduroam fallback)
│   │   │   ├── config.h
│   │   │   └── secrets.example.h  copy to secrets.h, add your WiFi login
│   │   └── telemetry/             reading every 10 s -> backend
│   │       ├── config.h           backend URL, interval, JSON keys
│   │       ├── sensors.cpp        plug real sensors in here
│   │       └── reading.h          reading -> JSON (unit-tested)
│   └── test/                      laptop unit tests
├── tools/mock_backend.py          fake backend for testing uploads
├── docs/
│   ├── BUILD_GUIDE.md             wiring + uploading, beginner-friendly
│   ├── TELEMETRY.md               data upload: JSON contract, settings, testing
│   ├── ROADMAP.md                 what's done, what's next
│   └── reference/                 schematic, user stories
└── .gitignore
```

## The files you'll actually edit

- `src/cultivator/config.h`: brightness, schedule, pins
- `src/telemetry/config.h`: backend address, upload interval
- `src/sign-on/secrets.h` (you create it): WiFi login
- `src/telemetry/sensors.cpp`: when a real sensor is added

## Hardware summary

| Part | Role |
|------|------|
| Arduino Nano ESP32 | The microcontroller ("brain") that runs the code |
| 12 V DC supply | Power for the LEDs |
| Red + Blue LED drivers (700 mA) | Feed each LED a safe, steady current |
| Red (660 nm) + Blue (440 nm) 3 W LEDs | The grow lights |
| 2× MOSFETs | Electronic switches the brain uses to dim each colour |
| DS3231 RTC | Battery-backed clock for the daily schedule |

Full details and wiring: [`docs/BUILD_GUIDE.md`](docs/BUILD_GUIDE.md).

## Status

`v0.3.0`: grow lights + WiFi + telemetry upload. It compiles, and the
JSON is unit-tested, but it has **not yet been tested on real hardware**. The
backend's `/api/logReading` route and the sensors are the next pieces to
connect. See [`docs/ROADMAP.md`](docs/ROADMAP.md).

## License

Not yet chosen. iGEM projects commonly use the MIT license — the team should
decide and add a `LICENSE` file.
