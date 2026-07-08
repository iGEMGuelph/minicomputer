# Cultivator — Grow-Light Controller

**iGEM Guelph 2026**

Firmware for the *Cultivator*: a controlled-environment device for growing
**duckweed** (a fast-growing aquatic plant used as sustainable protein/food/
fertilizer). This repository holds the code that runs on the device's
microcontroller.

> **What the code does today:** controls a two-colour (red + blue) LED grow
> light — sets each colour's brightness and runs them on a daily on/off
> schedule using a real-time clock. That's the current hardware. The larger
> product vision (sensors, app, dashboard) is tracked in
> [`docs/ROADMAP.md`](docs/ROADMAP.md).

---

## Quick start

👉 **New to this? Read [`docs/BUILD_GUIDE.md`](docs/BUILD_GUIDE.md).** It walks
you from a box of parts all the way to working, tested lights — no experience
assumed.

In short:
1. Wire the hardware (Section 3 of the build guide).
2. Install the Arduino IDE + RTClib library (Section 5).
3. Open `firmware/cultivator/cultivator.ino` and upload it (Section 5).
4. Open the Serial Monitor at 115200 and type `help`.

## Repository layout

```
minicomputer/
├── README.md                    <- you are here
├── firmware/
│   └── cultivator/
│       ├── cultivator.ino       the program that runs on the board
│       ├── config.h             ALL user-editable settings (pins, brightness, schedule)
│       └── schedule.h           the daily on/off time logic (kept separate, plain C++)
├── docs/
│   ├── BUILD_GUIDE.md           step-by-step wiring guide (plain, beginner-friendly)
│   ├── ROADMAP.md               what comes next (temp sensor, WiFi, app)
│   └── reference/               source materials (schematic, user stories)
└── .gitignore
```

## The one file you'll actually edit

`firmware/cultivator/config.h` — brightness, schedule, and which pins the
wires connect to. You should not need to touch anything else for normal use.

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

`v0.2.0` — grow-light firmware, ready for first hardware bring-up (not yet
tested on real hardware). Red/blue independent dimming, RTC daily schedule, and
a serial menu for hand testing. Scope is deliberately limited to what the V1.0
board actually has — see [`docs/ROADMAP.md`](docs/ROADMAP.md) for what's next
(temperature cutoff needs a real sensor first, then WiFi/app).

## License

Not yet chosen. iGEM projects commonly use the MIT license — the team should
decide and add a `LICENSE` file.
