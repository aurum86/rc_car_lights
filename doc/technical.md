# RC car lights — technical overview

Firmware for an **Arduino Nano** (ATmega328P-class) that reads up to three RC receiver PWM channels, optionally monitors battery voltage on an analog pin, and drives several LED (or MOSFET) outputs with PWM where the hardware allows.

## Control flow

Each `loop()` iteration:

1. **Inputs** — `pulseIn(..., HIGH)` on D2, D3, and D4 captures the high pulse width of each channel (microseconds). Battery is read with `analogRead(7)` (see pin note below).
2. **Low voltage** — If `LowVoltageDetector` decides the smoothed pack voltage is below the limit (default **6.8 V**, tuned for **2S** with a **÷2** divider), it flashes the turn outputs at ~1 Hz and **skips** all other logic for that pass.
3. **Otherwise** — Emergency modes on CH3 can force hazard-style blinking on the turn pins. If neither emergency band is active, **turn signals** use CH1 (steering) and CH2 (throttle) with a **3 s** dwell after throttle returns to “neutral.” **Headlights** (two behaviors), **backfire** exhaust pulse, and **brake / reverse** are derived from CH2 and CH3 thresholds defined in `rc_car_lights.ino`.

Modules are small `evaluate(...)` classes in the `.cpp` files included by the sketch; there is no separate build step beyond the Arduino IDE.

## CH3 (aux) pulse bands — one channel, several modes

All intervals are **open** (`low < pulse µs < high`), as in `Headlights` / `EmergencyLights` (`rc_car_lights.ino`).

### Timeline (typical servo-scale µs)

```
     1000      1200      1500      1600      1700      1900
       │         │         │         │         │         │
       ├─────────┤         │         │         │         │   Emergency 1  (1000…1200)
                 │         │         │         │         │
                 │◄─────── HL1 ─────────────────────────►│   Daylights    (1500…1900)
                 │         │         │                   │
                 │         │    ◄──── HL2 ──────────────►│   Xenon        (1600…1900)
                 │         │         │         │         │
                 │         │         │         ├─────────┤   Emergency 2  (1700…1900)
                 │         │         │         │         │
    ─────────────┴─────────┴─────────┴─────────┴─────────┴──► CH3 pulse width (µs)
```

`HL2` is fully inside `HL1`: xenon only turns on in the **upper** part of the daylight band.

### What is active in each band

| Region (µs) | Emergency (hazards) | HLights1 daylights | HLights2 xenon |
|-------------|---------------------|--------------------|----------------|
| 1000–1200 | Yes (`EmergencyLights`) | No | No |
| 1200–1500 | No | No | No |
| 1500–1600 | No | Yes | No |
| 1600–1700 | No | Yes | Yes |
| 1700–1900 | Yes (`…WithDaylights`) | Yes | Yes |

Between **1200** and **1500** µs, **none** of these CH3-driven lighting/emergency modes apply; **turn signals** still follow **CH1 + CH2** when low-voltage is not latched.

## Pin assignment (as in `rc_car_lights.ino`)

| Arduino label | Variable / role | Direction | Notes |
|---------------|-------------------|-----------|--------|
| **D2** | `pinCh1` | Input | Servo / steering PWM from receiver |
| **D3** | `pinCh2` | Input | Throttle PWM |
| **D4** | `pinCh3` | Input | Aux / mode PWM (headlights, emergency bands) |
| **A7** | `pinVoltageMetter` | Analog in | Battery sense (**not** D7 — see below) |
| **D5** | `pinLeft` | Output (PWM) | Left turn |
| **D6** | `pinRight` | Output (PWM) | Right turn |
| **D7** | `pinReverse` | Output | Reverse lamp drive (`analogWrite`; see PWM note) |
| **D8** | `pinExhaust` | Output | “Backfire” / exhaust LED |
| **D9** | `pinBreak` | Output (PWM) | Brake lamp |
| **D10** | `pinLights2` | Output (PWM) | “Xenon” channel (blink on/off) |
| **D11** | `pinLights1` | Output (PWM) | Daylight fade with rear |
| **D12** | `pinLightsR` | Output | Rear red / tail (faded with D11 in software) |

### Analog channel vs digital pin 7

On the Nano, **`analogRead(7)` reads physical pin A7** (ADC channel 7). **`pinReverse` uses digital D7**. They are different pins; do not tie battery sense to D7.

### PWM behavior

Hardware PWM on the Nano: **D5, D6, D9, D10, D11** (and D3 if unused). **D7, D8, D12** are not hardware-PWM pins; `analogWrite` on those resolves to full **ON** for any non-zero duty cycle, so brightness curves on D7/D8/D12 are effectively on/off unless you change pins or use soft-PWM.

## Wiring (high level)

- **Receiver** — Connect **signal** wires for the three used channels to **D2, D3, D4**. Tie **GND** between receiver, ESC, and Arduino. Keep **5 V** to the receiver only if your receiver is 5 V-tolerant and your supply is clean (many installs use the BEC from the ESC).
- **Battery monitor** — Scale pack voltage so the Arduino analog pin never exceeds **5 V**. For **2S**, a **two-resistor divider** (e.g. two **1 kΩ** as in the project README) from pack+ to GND, **midpoint to A7**, is appropriate. Common **GND** with the pack and Arduino.
- **Loads** — Arduino pins source limited current. For real automotive or high-power LEDs, drive **gates of N-channel MOSFETs** (or low-side switches) from these pins, with **flyback** where needed, **current limiting** on LEDs, and a **common ground** with the Arduino.

## Calibration

Pulse width thresholds (`Headlights`, `Turns`, `EmergencyLights`, `BackFire`, `BreakReverseState`, `LowVoltageDetector`) are set for a **Sanwa RX472**-style channel timing. Set `Debug` to `true`, use **9600 baud** Serial, and adjust the constants in the `INITIALIZATION` section of `rc_car_lights.ino` if your receiver’s neutral and endpoints differ.

## Dependencies

- **[NeoTimer](https://github.com/jrullan/neotimer)** — non-blocking blink timing for turn / hazard / low-voltage patterns.
