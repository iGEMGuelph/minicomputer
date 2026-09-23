#!/usr/bin/env python3
"""
mock_backend.py  —  a stand-in for the real backend, for testing the board.

It accepts the same POST the real backend will, checks the JSON matches the
contract in docs/TELEMETRY.md, prints it, and answers 201 (or 400 if wrong).
Nothing is saved. Only needs Python 3 (no installs).

    python3 tools/mock_backend.py              # listens on port 8000
    python3 tools/mock_backend.py --port 8001  # if the real backend uses 8000

Then set BACKEND_URL in firmware/cultivator/src/telemetry/config.h to
http://<this computer's IP>:<port>/api/logReading and upload the firmware.
The board and this computer must be on the same WiFi network.
"""

import argparse
import json
import re
from datetime import datetime
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

PATH = "/api/logReading"
NUMBER_KEYS = ("pH", "Biomass", "AirTemp", "WaterTemp")
TIME_RE = re.compile(r"^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}(Z|[+-]\d{2}:\d{2})?$")


def validate(data):
    """Return a list of problems with one reading (empty list = valid)."""
    if not isinstance(data, dict):
        return ["body is not a JSON object"]
    problems = []
    expected = {"Time", *NUMBER_KEYS}
    if set(data) != expected:
        problems.append(f"keys are {sorted(data)}, expected {sorted(expected)}")
    t = data.get("Time")
    if t is not None and not (isinstance(t, str) and TIME_RE.match(t)):
        problems.append(f"Time {t!r} is not ISO 8601 (YYYY-MM-DDTHH:MM:SS)")
    for key in NUMBER_KEYS:
        v = data.get(key)
        if v is not None and (isinstance(v, bool) or not isinstance(v, (int, float))):
            problems.append(f"{key} {v!r} is not a number or null")
    return problems


class Handler(BaseHTTPRequestHandler):
    def do_POST(self):
        if self.path != PATH:
            return self.reply(404, {"detail": f"use POST {PATH}"})
        raw = self.rfile.read(int(self.headers.get("Content-Length", 0)))
        try:
            data = json.loads(raw)
        except json.JSONDecodeError:
            print(f"[{now()}] BAD JSON from {self.client_address[0]}: {raw!r}", flush=True)
            return self.reply(400, {"detail": "invalid JSON"})
        problems = validate(data)
        status = "OK " if not problems else "BAD"
        print(f"[{now()}] {status} {self.client_address[0]}  {json.dumps(data)}", flush=True)
        for p in problems:
            print(f"           - {p}", flush=True)
        if problems:
            return self.reply(400, {"detail": problems})
        self.reply(201, {"status": "stored (mock - not really)"})

    def do_GET(self):
        self.reply(200, {"status": "mock backend running", "post_to": PATH})

    def reply(self, code, body):
        payload = json.dumps(body).encode()
        self.send_response(code)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(payload)))
        self.end_headers()
        self.wfile.write(payload)

    def log_message(self, *args):
        pass  # we print our own, cleaner log lines


def now():
    return datetime.now().strftime("%H:%M:%S")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[1])
    parser.add_argument("--port", type=int, default=8000)
    port = parser.parse_args().port
    print(f"Mock backend listening on http://0.0.0.0:{port}{PATH}  (Ctrl+C to stop)", flush=True)
    ThreadingHTTPServer(("0.0.0.0", port), Handler).serve_forever()
