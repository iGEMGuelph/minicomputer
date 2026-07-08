# Roadmap

The [User Stories PDF](reference/user-stories.pdf) describes a full product.
This repository currently implements only the first, foundational piece — the
grow-light control that the V1.0 board actually supports. This file tracks what
exists and what's next, so future team members know where to pick up.

## Done — v0.2.0 (current hardware)
- [x] Red + blue LED **independent brightness** control (PWM dimming via MOSFETs)
- [x] **Daily light schedule** (RTC clock), including windows that cross midnight
- [x] **Serial command interface** for live testing/tuning

## Next up (small additions to the current board)
- [ ] **Over-temperature cutoff** — a "Must" user story, but it needs a real
      temperature sensor added to the board first (the V1.0 schematic shows the
      control path but *not* a sensor). Once a sensor exists, read it and force
      the lights off above a threshold. **Deliberately NOT faked in software**
      (e.g. using the clock chip's internal temperature would be misleading).
- [ ] **Auto-restart / watchdog** for 24/7 stability (an "Engineer/Should"
      story) — ESP32 task watchdog. Left out for now to avoid shipping a
      version-sensitive feature that can't be tested before hardware exists.
- [ ] Add the other two light colours from the PDF (far-red, UV-B) — same
      driver + MOSFET pattern, just two more channels
- [ ] Save schedule/brightness settings so they survive a power cycle
      (ESP32 "Preferences" / flash storage)
- [ ] Read the 12 V supply voltage via the "Split 12V" divider already on the
      schematic (basic power monitoring)

## Bigger (needs new hardware not yet on the board)
- [ ] **Sensors** — water temperature, pH, CO₂, light, duckweed biomass,
      aeration/turbidity. None are wired yet; each needs its own sensor part.
- [ ] **WiFi + data upload** — the Arduino Nano ESP32 has WiFi built in. Send
      readings to a database/dashboard (the PDF's "API calls" idea).
- [ ] **App / dashboard** — templated crop profiles, alerts, historical
      metrics, saved/favourited settings. This is a separate software project.

## Notes for future contributors
- Keep all user-editable settings in `config.h`.
- Keep pure logic (like the schedule math in `schedule.h`) free of Arduino
  calls so it stays simple and reviewable.
- **Don't fake hardware in software.** If a feature needs a part that isn't on
  the board, add it here as future work rather than approximating it — an
  approximation that behaves unexpectedly is worse than an honest "not yet."
- If the RTC is missing, AUTO mode holds the lights **off** and says so; manual
  commands still work. Fail safe, not surprising.
- Update this file and the version number in `cultivator.ino` when you ship
  a change.
