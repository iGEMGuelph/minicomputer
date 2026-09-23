# Telemetry — sending readings to the backend

Every **10 seconds** the board takes a reading and sends it over WiFi to the
backend as JSON. The backend stores it in the database, and the dashboard graphs
it.

```
 Board (this repo)          Backend (2026project)     Supabase        Dashboard
 RTC + sensors ──POST JSON──►  sanitise + store  ───►  last N rows ───►  graphs
 every 10 s                    /api/logReading
```

**Status right now:** the clock (RTC) is the only data source on the board, so
`Time` is filled in and the four sensor values are always `null`. The whole
pipeline (reading, buffering, WiFi, HTTP) is built and tested, so a new sensor
only has to be plugged in at one spot (see [Adding a sensor](#adding-a-sensor)).

---

## The message (contract with the backend)

`POST <BACKEND_URL>` with `Content-Type: application/json`, one reading per
request:

```json
{ "Time": "2026-09-23T16:14:00", "pH": null, "Biomass": null, "AirTemp": null, "WaterTemp": null }
```

| Key | Type | Meaning | Range | Decimals |
|-----|------|---------|-------|----------|
| `Time` | string, ISO 8601 | When the reading was taken, from the RTC | — | seconds |
| `pH` | number or `null` | Acidity of the water | 0 – 14 | 1 |
| `Biomass` | number or `null` | Duckweed amount as a proportion | 0 – 1 | 2 |
| `AirTemp` | number or `null` | Air temperature, °C | — | 2 |
| `WaterTemp` | number or `null` | Water temperature, °C | — | 2 |

Guarantees from the board:
- All five keys are always present.
- `null` means "no reading" (sensor not installed, or it failed). The board
  never sends a made-up value, `NaN` or `Infinity`.
- The board does **not** range-check values. A broken sensor could send pH 20.
  The backend sanitises the data and the dashboard shows the error.
- `Time` has **no timezone** by default: it's whatever the RTC is set to (local
  time). Change `TIME_UTC_OFFSET` in `config.h` to append `Z` or `-04:00`.

What the board expects back:
- Any **2xx** status counts as success. The response body is ignored.
- Anything else (or no answer within `HTTP_TIMEOUT_MS`) counts as failure. It's
  logged on the Serial Monitor and the reading is **not** retried, since the
  next one is only 10 s away.
- If `BACKEND_API_KEY` is set in `secrets.h`, it's sent in the `X-API-Key`
  header (header name set by `API_KEY_HEADER`).

### For the backend team: suggested endpoint

The backend route doesn't exist yet. The board posts to `/api/logReading` by
default (changeable in `config.h`). Here's a minimal FastAPI starting point
matching this contract. It's only a suggestion; the backend team owns the real
one:

```python
from datetime import datetime
from pydantic import BaseModel

class Reading(BaseModel):
    Time: datetime
    pH: float | None = None
    Biomass: float | None = None
    AirTemp: float | None = None
    WaterTemp: float | None = None

@app.post(f"{API_PREFIX}/logReading", status_code=201)
def log_reading(reading: Reading):
    ...  # sanitise, insert into Supabase, enforce the row limit
```

---

## Settings — `firmware/cultivator/src/telemetry/config.h`

| Setting | Default | What it does |
|---------|---------|--------------|
| `TELEMETRY_ENABLED` | `1` | `0` turns uploading off completely |
| `BACKEND_URL` | `http://192.168.1.100:8000/api/logReading` | **Must change.** Where readings go |
| `TELEMETRY_INTERVAL_MS` | `10000` | Time between readings |
| `HTTP_TIMEOUT_MS` | `5000` | Give up on a request after this long |
| `TELEMETRY_QUEUE_LEN` | `30` | Readings kept while WiFi is down (30 × 10 s = 5 min) |
| `KEY_*` | `Time`, `pH`, … | JSON key names |
| `DECIMALS_*` | 1 / 2 / 2 / 2 | Decimal places per value |
| `TIME_UTC_OFFSET` | `""` | Timezone suffix for `Time` |
| `TELEMETRY_LOG_EACH_SEND` | `1` | Print every POST on the Serial Monitor |

**`BACKEND_URL` can't be `localhost`.** On the board, `localhost` means the
board itself. Use the IP address of the computer running the backend
(`ipconfig getifaddr en0` on Mac, `ipconfig` on Windows). The board and that
computer must be on the same network. The backend already listens on `0.0.0.0`,
which allows this.

Secrets (API key, HTTPS certificate) go in `src/telemetry/secrets.h`: copy
`secrets.example.h` to `secrets.h`. That file is gitignored, and it's optional.
Without it the board sends no key.

**WiFi** is not configured here. It's handled by the sign-on module
(`src/sign-on/`, login in `src/sign-on/secrets.h`). Telemetry only checks
whether WiFi is connected.

---

## How it works (for whoever maintains this)

Files in `firmware/cultivator/src/telemetry/`:

| File | Role |
|------|------|
| `config.h` | All settings (above) |
| `sensors.h` / `sensors.cpp` | **One function per sensor.** Returns a value, or `NAN` |
| `reading.h` | The `Reading` struct and JSON builder. Pure C++, unit-tested |
| `telemetry.cpp` | Timing, buffering, HTTP upload, serial commands |
| `secrets.example.h` | Template for the optional API key / certificate |

The work is split in two so **the grow lights never wait on the network**:

1. **Sampling (main `loop()`)**: a `millis()` timer fires every 10 s, reads
   the RTC and sensors, and puts the reading into a FreeRTOS queue. This takes
   microseconds. The timer schedules from the previous target rather than
   "now", so the 10 s rhythm doesn't drift.
2. **Uploading (a separate FreeRTOS task)**: sleeps until a reading is in the
   queue, then POSTs it. An HTTP request can take seconds on slow WiFi. Because
   it runs in its own task, the lights and serial menu keep running.

We don't use a `while` loop with `delay()` (it would freeze everything else), and
we don't use a timer interrupt or `Ticker` (network calls aren't allowed inside
those).

**WiFi dropouts:** readings stay in the queue until WiFi is back, then get sent
in order. If the queue fills up, the oldest reading is dropped. **No RTC:**
nothing is sent, because a reading without a time is useless. A warning is
printed once.

---

## Adding a sensor

All sensor code goes in `sensors.h` / `sensors.cpp`. For example, for water
temperature:

1. Wire it and set `WATER_TEMP_SENSOR_PIN` in `sensors.h`.
2. Add the sensor's library to `lib_deps` in `platformio.ini` if it needs one.
3. Initialise it in `sensorsSetup()`.
4. In `readWaterTemp()`, replace `return NAN;` with the real reading, in °C.
   **Return `NAN` if the read fails.** Never return a fallback number.

That's it: it'll be sent as `WaterTemp` from then on. Keep each read fast
(well under a second), because it runs on the main loop.

---

## Testing

**Unit tests (no board needed)** check the exact JSON produced:
```
cd firmware/cultivator
pio test -e native
```

**Without the real backend**, use the mock backend. It prints every reading and
checks it matches the contract above:
```
python3 tools/mock_backend.py            # port 8000 (use --port 8001 if taken)
```
Set `BACKEND_URL` to `http://<your laptop IP>:8000/api/logReading`, upload,
and you should see a line every 10 s.

**On the board**, use these Serial Monitor commands (115200 baud):

| Command | What it does |
|---------|--------------|
| `net` | WiFi status, backend URL, counts of sent / failed / dropped, last HTTP result |
| `payload` | Print the JSON a reading would send right now (works without WiFi) |
| `send` | Take a reading immediately and upload it |
| `telemetry off` / `telemetry on` | Pause / resume uploads (until reboot) |

Common problems:

| You see | Cause |
|---------|-------|
| `backend unreachable (connection refused)` | Wrong IP/port in `BACKEND_URL`, backend not running, or a firewall on the backend computer |
| `backend answered HTTP 404` | Backend is running but has no `/api/logReading` route yet |
| `backend answered HTTP 422` | Backend rejected the JSON shape. Compare its model with the contract above |
| `RTC not found - no readings will be sent` | Check the clock wiring (Build Guide Step 3) |
| `net` shows WiFi NOT connected | See the sign-on messages. Check `src/sign-on/secrets.h` |
