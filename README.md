# Cultivator — Grow-Light Controller

**iGEM Guelph 2026**

Firmware for the *Cultivator*: a controlled-environment device for growing
**duckweed** (a fast-growing aquatic plant used as sustainable protein/food/
fertilizer). This repository holds the code that runs on the device's
microcontroller.

> **What the code does today:** controls a two-colour (red + blue) LED grow
> light — sets each colour's brightness and runs them on a daily on/off
> schedule using a real-time clock — and takes its brightness from the Lumifert
> dashboard over WiFi. Four channels exist in software (UVB, red, far red,
> blue); only red and blue have drivers on the V1.0 board, and the other two
> report themselves as "not wired". The larger product vision (sensors, more
> colours) is tracked in [`docs/ROADMAP.md`](docs/ROADMAP.md).

---

## Quick start

> For the full dashboard-to-LED procedure, including the phone-hotspot
> networking and every problem hit on the first live test, see
> `docs/DIMMER_RUNBOOK.md` in the `2026project` repo.

👉 **New to this? Read [`docs/BUILD_GUIDE.md`](docs/BUILD_GUIDE.md).** It walks
you from a box of parts all the way to working, tested lights — no experience
assumed.

In short:
1. Wire the hardware (Section 3 of the build guide).
2. Install [PlatformIO](https://platformio.org/install/cli) (`pip install platformio`).
3. Copy `firmware/cultivator/src/sign-on/secrets.h.example` to `secrets.h` in
   the same folder and fill in your WiFi details. The build fails without it.
4. Point the board at your dashboard: set `API_HOST` in
   `firmware/cultivator/src/remote/config.h` to the IP of the machine running
   the backend.
5. Build and upload:
   ```
   cd firmware/cultivator
   pio run --target upload
   pio device monitor
   ```
6. Type `help` in the monitor.

## Repository layout

```
minicomputer/
├── README.md                    <- you are here
├── firmware/
│   └── cultivator/
│       ├── platformio.ini       build config (board, libraries, pinned platform)
│       └── src/
│           ├── main.cpp         the only setup()/loop(); calls the three modules
│           ├── cultivator/      grow-light control
│           │   ├── cultivator.cpp
│           │   ├── cultivator.h     what other modules may call
│           │   ├── config.h         ALL user-editable settings (pins, brightness, schedule)
│           │   └── schedule.h       the daily on/off time logic (plain C++, no Arduino)
│           ├── remote/          follows the dashboard over WiFi
│           │   ├── remote.cpp
│           │   └── config.h         API_HOST — the address of the dashboard backend
│           └── sign-on/         connects to WiFi (private network, then eduroam)
│               ├── sign_on.cpp
│               ├── config.h
│               └── secrets.h.example   copy to secrets.h and fill in (gitignored)
├── docs/
│   ├── BUILD_GUIDE.md           step-by-step wiring guide (plain, beginner-friendly)
│   ├── ROADMAP.md               what comes next (temp sensor, more colours)
│   └── reference/               source materials (schematic, user stories)
└── .gitignore
```

## The two files you'll actually edit

* `firmware/cultivator/src/cultivator/config.h` — brightness, schedule, and
  which pins the wires connect to.
* `firmware/cultivator/src/remote/config.h` — `API_HOST`, the address of the
  machine running the dashboard backend. This is the one value that changes
  when you move the device to a different network.

You should not need to touch anything else for normal use.

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

`v0.3.0` — grow-light firmware, ready for first hardware bring-up (not yet
tested on real hardware). Red/blue independent dimming, RTC daily schedule, a
serial menu for hand testing, and dashboard control over WiFi. Four channels are
defined in software so UVB and far red can be fitted later with only a
`config.h` change; on the V1.0 board they report as "not wired". See
[`docs/ROADMAP.md`](docs/ROADMAP.md) for what's next (the temperature cutoff
still needs a real sensor).

### How dashboard control works

The board does **not** run a web server. It reaches out to the backend every two
seconds and asks what the lights should be. That direction is deliberate:
outbound connections work on networks that block one device from connecting to
another (eduroam does exactly this), and nothing has to track the board's
address.

Three modes, shown by the `status` command:

| Mode | Brightness comes from | How you get there |
|------|----------------------|-------------------|
| `auto` | the RTC daily schedule | power-on default; `auto` |
| `remote` | the dashboard | automatic on the first successful poll |
| `manual` | the serial menu | typing any colour command, or `manual` |

Typing a colour at the serial console takes control away from the dashboard —
the person standing at the hardware wins, and the dashboard shows "read only"
until you type `remote`. If the backend becomes unreachable the lights **hold**
their last levels; only after five minutes do they fall back to the schedule.
Going dark the moment WiFi hiccups would be much worse for the plants.

## License

Not yet chosen. iGEM projects commonly use the MIT license — the team should
decide and add a `LICENSE` file.
