# ESP32 RC Car — 2WD Motor Driver

A simple, modular 2-wheel drive RC car project for the **ESP32-DevKitC V2** using an **L298N** motor driver and Arduino IDE.

---

## Hardware Requirements

| Component | Details |
|---|---|
| ESP32 | DevKitC V2 (WROOM-32) |
| Motor driver | L298N (dual H-bridge) |
| Motors | 2x DC gear motors (3–6V recommended) |
| Power | Battery pack (e.g. 2x 18650 or 4xAA) |
| Chassis | 2-wheel robot chassis |

**Wiring diagram:**

```
L298N Module         ESP32 DevKitC V2
─────────────────────────────────────
IN1          ──►     GPIO 26
IN2          ──►     GPIO 27
ENA          ──►     GPIO 14
IN3          ──►     GPIO 25
IN4          ──►     GPIO 33
ENB          ──►     GPIO 32
GND          ──►     GND

L298N Power:
  +12V  ◄──  Battery (+)
  GND   ◄──  Battery (-) + ESP32 GND
  +5V   ◄──  L298N onboard regulator (optional power for ESP32)
```

> **Note:** If your motors spin the wrong direction, swap the IN1/IN2 (or IN3/IN4) values for that motor in `pinout.h`.

---

## Project Structure

```
main/
├── main.ino      # Entry point — setup() and loop()
├── drive.ino     # Motor control functions
└── pinout.h      # Pin definitions and ESP32 GPIO reference
```

### `pinout.h` — Pin Definitions

All L298N pin assignments live here. Change a number and it updates everywhere.

| Define | GPIO | L298N Pin | Purpose |
|---|---|---|---|
| `LEFT_IN1` | 26 | IN1 | Left motor direction A |
| `LEFT_IN2` | 27 | IN2 | Left motor direction B |
| `LEFT_EN` | 14 | ENA | Left motor speed (PWM) |
| `RIGHT_IN1` | 25 | IN3 | Right motor direction A |
| `RIGHT_IN2` | 33 | IN4 | Right motor direction B |
| `RIGHT_EN` | 32 | ENB | Right motor speed (PWM) |

The file also includes a full ESP32-DevKitC V2 (WROOM-32) GPIO reference listing safe pins, input-only pins, flash pins to avoid, and boot/strapping pins.

### `drive.ino` — Motor Control

Contains all the low-level and high-level motor functions. No pins are hardcoded here — they come from `pinout.h`.

### `main.ino` — Your Code

This is where you write your movement logic. The `loop()` calls functions from `drive.ino`.

---

## Setup

1. Install [Arduino IDE](https://www.arduino.cc/en/software)
2. Add ESP32 board support:
   - Go to **File > Preferences**
   - In **Additional Boards Manager URLs** add:
     ```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```
   - Go to **Tools > Board > Boards Manager**
   - Search for **esp32** and install **esp32 by Espressif Systems**
3. Connect ESP32 via USB
4. Select board: **Tools > Board > ESP32 Arduino > ESP32 Dev Module**
5. Select the correct COM port under **Tools > Port**
6. Open `main/main.ino`
7. Click **Upload**

---

## Functions Reference

### `driveBegin()`

Call once in `setup()`. Sets all motor pins to OUTPUT and stops the motors.

```cpp
void setup() {
  Serial.begin(115200);
  driveBegin();
}
```

### `drive(int forwardBack, int leftRight)`

Main movement function. Both parameters are **0–100**, with **50 = center** (stop/dead zone).

```cpp
drive(100, 50);   // full forward
drive(0, 50);     // full reverse
drive(50, 100);   // spin right in place
drive(50, 0);     // spin left in place
drive(100, 100);  // diagonal: forward + veer right
drive(0, 0);      // diagonal: reverse + veer left
drive(50, 50);    // stop (coast)
```

| forwardBack | leftRight | Result |
|---|---|---|
| 100 | 50 | Full forward |
| 0 | 50 | Full reverse |
| 50 | 100 | Spin right in place |
| 50 | 0 | Spin left in place |
| 100 | 100 | Forward + veer right |
| 100 | 0 | Forward + veer left |
| 0 | 100 | Reverse + veer right |
| 0 | 0 | Reverse + veer left |
| 50 | 50 | Stop (coast) |

Commands are **non-blocking** — motors hold the last command until a new one is sent.

### `stop()`

Hard brake. Both IN pins go HIGH on each motor, shorting the windings for instant stop. Faster than coasting to a stop with `drive(50, 50)`.

```cpp
stop();  // instant brake
```

### `dance(int pause, int spd)`

Runs a pre-programmed dance sequence and returns to the starting position. The `pause` parameter sets the **milliseconds** to stop between each move. The `spd` parameter sets the motor speed (0–100).

```cpp
dance(1000, 100);  // dance with 1-second pauses, full speed
dance(500, 60);    // faster dance, 60% speed
```

The sequence:
1. Forward
2. Spin right 360°
3. Reverse
4. Spin left 360°
5. Forward + right diagonal
6. Back + left diagonal (returns to start)

Each move takes `pause` ms, then stops for `pause` ms before the next move. The function is **blocking** — it runs the full sequence before returning.

---

## Customizing the Dance

Edit the `dance()` function in `drive.ino` to change the sequence. Each move is a `drive()` call + `delay()` + `stop()` block:

```cpp
void dance(int pause) {
  // Forward
  drive(100, 50);
  delay(pause);
  stop();
  delay(pause);

  // Spin right 360
  drive(50, 100);
  delay(pause);
  stop();
  delay(pause);

  // Add your own moves here
  // ...

  // Last move (no stop delay needed after the final move)
  drive(1, 1);
  delay(pause);
  stop();
}
```

---

## ESP32-DevKitC V2 GPIO Quick Reference

### Safe to Use (always)
```
GPIO 13, 16, 17, 18, 19, 21, 22, 23, 32, 33
```

### Safe but ADC2 (analogRead fails when Wi-Fi is ON)
```
GPIO 4, 25, 26, 27
```

### Input Only (no output, no pull-up/down)
```
GPIO 34, 35, 36 (VP), 39 (VN)
```

### Do Not Use (SPI flash — will crash)
```
GPIO 6, 7, 8, 9, 10, 11
```

### Boot / Strapping Pins (usable but affect boot — avoid if possible)
```
GPIO 0   — Boot button (must be HIGH to run firmware)
GPIO 2   — Onboard LED, **used by Wi-Fi** (must be LOW at boot)
GPIO 5   — Must be HIGH at boot
GPIO 12  — **Used by Wi-Fi**, must be LOW at boot (HIGH can damage flash)
GPIO 15  — **Used by Wi-Fi**, must be HIGH at boot
```

### Serial (leave free unless needed)
```
GPIO 1  (TX0), GPIO 3 (RX0) — used for USB serial upload
```

### Avoid with Wi-Fi / Bluetooth
```
GPIO 2, 12, 15 — radio/strapping shared
ADC2 pins (2, 4, 12-15, 25-27) — analogRead() fails when Wi-Fi active
```

---

## Troubleshooting

| Problem | Solution |
|---|---|
| Motors don't spin | Check wiring, ensure L298N has power (12V + GND), check ENA/ENB jumpers are on |
| One wheel spins wrong way | Swap IN1/IN2 (or IN3/IN4) values for that motor in `pinout.h` |
| ESP32 resets randomly | Power supply too weak — use separate battery for motors, not just USB |
| Motors jitter at stop | Normal for some L298N modules — `stop()` hard brake minimizes this |
| Upload fails | Hold BOOT button on ESP32 during upload, or select correct COM port |
| `analogWrite` not working | Ensure you have ESP32 Arduino core v2.x+ installed |

---

## License

MIT
