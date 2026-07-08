# Build Guide — Connecting the Hardware

This is a step-by-step guide to **physically wiring the parts together**, one
connection at a time. No experience needed — just follow it in order and do the
✅ check at the end of each step before moving on.

**How to read a step:** each connection is written as

> **FROM part / pin  →  TO part / pin**

Read it as "run a wire from the first thing to the second thing."

> ⚠️ **Two safety rules for the whole build:**
> 1. Keep the **12 V supply UNPLUGGED** until the very last section. We wire
>    everything with no power connected.
> 2. The LEDs are **bright 3 W** — once powered, **don't look straight into them.**

---

## Section 0 — What you should already have wired vs. what this guide covers

Your schematic (in [`reference/`](reference/)) already shows the **light chains**
correctly: the 12 V supply, the two LED drivers, the two LEDs, and the two MOSFET
switches. **Copy the schematic for those parts.**

What people get wrong is **which Arduino pin each wire goes to**. So the most
important part of this guide is **Section 3: the exact Arduino pins.** Those pins
must match the code. If you follow nothing else carefully, follow that.

---

## Section 1 — Lay out your parts

Put these on the table and tick them off. If something is missing, stop and get
it before wiring.

- [ ] Arduino Nano ESP32 (the "brain")
- [ ] 12 V DC supply + its connector (the `KH-GX20-2P`)
- [ ] Blue LED driver (700 mA) + Blue LED (440 nm, 3 W)
- [ ] Red LED driver (700 mA) + Red LED (660 nm, 3 W)
- [ ] 2 × N-channel MOSFETs (the switches — `Q1` blue, `Q2` red on the schematic)
- [ ] DS3231 clock module (+ its coin battery installed)
- [ ] Resistors: 2 × **220 Ω** and 2 × **10 kΩ**
- [ ] USB-C cable for the Arduino
- [ ] Breadboard + jumper wires (or however you're mounting it)

---

## Section 2 — The idea, in 30 seconds

Each colour is a simple chain that power flows through in a line:

```
12 V (+) ──► DRIVER ──► LED ──► MOSFET ──► GND (−)
                                  ▲
                                  │  the Arduino taps here
                          Arduino pin (D2 or D3)
```

- The **driver** limits the current so the LED can't be over-driven — it is
  safe at 700 mA no matter what.
- The **MOSFET** is a switch. The Arduino flips it on/off very fast, and that
  fast on/off is what dims the LED.
- **Red and blue are two identical copies of this chain.** That's the whole system.

---

## Section 3 — Wire it, one connection at a time

### 🧱 Step 1 — Build the two light chains (copy the schematic)

Do this once for **blue**, then again for **red**, using your schematic as the map:

1. **12 V (+)  →  driver `IN`**
2. **driver `OUT`  →  LED `+`** (the LED's longer leg / marked-positive side)
3. **LED `−`  →  MOSFET drain leg** (check your MOSFET's pinout for which leg is "drain")
4. **MOSFET source leg  →  the common ground rail** (you'll join grounds in Step 5)
5. Place the **220 Ω** resistor in the wire going to the MOSFET **gate** leg
   (this is the control wire that will go to the Arduino).
6. Place the **10 kΩ** resistor from the **gate leg to ground** (this holds the
   switch OFF by default).

> 🔧 Why those two resistors: the **220 Ω** is a protective "speed bump" between
> the Arduino and the switch. The **10 kΩ** keeps the light **off by default**,
> even before the code runs — this is an important safety detail, don't skip it.

✅ **Check:** you have two complete light chains. Each ends at a MOSFET whose
**gate has a 220 Ω resistor** sticking out, ready to connect to the Arduino.
**Nothing touches the Arduino yet.**

---

### 🧱 Step 2 — Connect the two control wires to the Arduino

⭐ **This is the most important step. These pins must match the code exactly.**
Use the pin **labels printed on the Arduino Nano ESP32 board.**

| Wire (from the 220 Ω on the gate) | Goes to Arduino pin |
|-----------------------------------|:-------------------:|
| **Blue** MOSFET gate              | **`D2`**            |
| **Red** MOSFET gate               | **`D3`**            |

✅ **Check:** exactly two wires touch the Arduino so far — blue gate → `D2`,
red gate → `D3`.

---

### 🧱 Step 3 — Connect the clock (4 wires)

The DS3231 clock module has 4 pins. Wire each one to the Arduino:

| Clock pin | Goes to Arduino pin |
|-----------|:-------------------:|
| `SDA`     | **`A4`**            |
| `SCL`     | **`A5`**            |
| `VCC`     | **`3V3`**  (powers the clock) |
| `GND`     | **`GND`**           |

✅ **Check:** four wires between the clock and the Arduino, matching the table.

---

### 🧱 Step 4 — Join ALL the grounds together ⚠️ (the step people forget)

"Ground" is the shared return path for electricity. **Every** ground must meet
at one common point — one rail on your breadboard, or one junction. Connect all
of these together:

1. Arduino `GND`
2. The **12 V supply's minus (−)**
3. **Blue** MOSFET source leg
4. **Red** MOSFET source leg
5. Clock `GND` (if it isn't already joined at the Arduino's `GND`)

✅ **Check:** pick any ground wire and trace it — it should be able to reach
every other ground. On the schematic this shared point is labelled
**"Common ground."**
👉 **If you skip this, nothing will switch on.** It is the #1 first-build mistake.

---

### 🧱 Step 5 — Power for the Arduino (USB only, for now)

The Arduino runs on **5 V**, which is completely separate from the 12 V lights.

- For testing, just plug the **USB-C cable into your laptop.** That alone powers
  the Arduino. You do **not** need to connect anything to the `5V` pin yet.
- (For standalone use later, a regulated **5 V** can be fed into the `5V` pin.)

✅ **Check:** USB-C cable ready. **12 V still unplugged.**

---

### ✅ Full wiring checklist (before any power)

- [ ] Both light chains built by copying the schematic (Step 1)
- [ ] Blue gate → **`D2`** (Step 2)
- [ ] Red gate → **`D3`** (Step 2)
- [ ] Clock: `SDA`→**`A4`**, `SCL`→**`A5`**, `VCC`→**`3V3`**, `GND`→**`GND`** (Step 3)
- [ ] **All grounds joined** at one common point (Step 4)
- [ ] USB-C ready, **12 V still unplugged** (Step 5)

> 🔁 **If you had to use different pins** than `D2`/`D3`/`A4`/`A5`, that's OK — the
> pins are set in one file, `firmware/cultivator/config.h`, and it's a 4-line
> change. Just write down which pins you actually used and pass that along before
> the code is uploaded.

---

## Section 4 — Powering on and off (the safe order)

There are two separate plugs: **USB (5 V) for the brain** and **12 V for the
lights.** The order matters. The rule is always:

> **Brain first ON, brain last OFF.**

**To turn ON:**
1. Plug in **USB (5 V)** first — the brain wakes up. It always starts with both
   lights **off**, then follows the daily schedule.
2. *Then* plug in the **12 V** supply — the lights can now light up.

**To turn OFF:**
1. Unplug the **12 V** supply first — the lights go dark.
2. *Then* unplug **USB (5 V)** — the brain powers down last.

Powering the brain first means it is always in control before the 12 V arrives,
so the lights never come on uncontrolled.

> ℹ️ If the current time is inside the "on" part of the schedule, the lights
> **will** come on (at their daytime brightness) as soon as you connect 12 V —
> that's normal. For your **very first test**, type `off` in the Serial Monitor
> before connecting 12 V so you can bring each colour up gently (see Section 5).

---

## Section 5 — Uploading the code (short)

Once it's wired, the code goes onto the board from a laptop using the free
**Arduino IDE**:

1. Install the **Arduino IDE** (arduino.cc/en/software).
2. In the IDE, add the **Arduino Nano ESP32** board and the **RTClib** library
   (both through the IDE's built-in installers — Boards Manager and Library
   Manager).
3. Open `firmware/cultivator/cultivator.ino`, plug the board in over USB, pick
   the board/port, and click **Upload**.
4. Open the **Serial Monitor** at **115200 baud** and type `help`.

**First test — go slow and start dim:** before connecting 12 V, type `off` in
the Serial Monitor (this holds both colours off). *Then* connect 12 V and test
`red 10`, then `blue 10` — confirm each colour responds at low brightness before
turning it up. When you're happy, type `auto` to hand control to the daily
schedule.
