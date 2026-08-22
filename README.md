# ESP32 RC Car — 2WD Motor Driver

A modular 2-wheel drive RC car for **ESP32-DevKitC V2** with **L298N** motor driver, featuring acceleration ramping, progressive steering, battery monitoring, and motor overvoltage protection.

---

## Hardware Requirements

| Component | Details |
|---|---|
| ESP32 | DevKitC V2 (WROOM-32) |
| Motor driver | L298N (dual H-bridge) |
| Motors | 2× DC gear motors (3–6V, "TT" yellow motors) |
| Battery | 2S Li-ion (2× 18650, 7.4V nom / 8.4V max / 6.0V min) |
| Buck converter | LM2596 (set to **5.0V** for ESP32) |
| Chassis | 2WD robot chassis |

---

## Wiring

```
BATTERY (2S Li-ion)
   │
   ├─(+) ──► LM2596 IN+          BATTERY (+) ──► L298N +12V
   │                                 │
   └─(−) ──► LM2596 IN− ──┬────── BATTERY (−) ──► L298N GND ──┬──► ESP32 GND
                          │                                    │
LM2596 OUT+ (5.0V) ──► ESP32 5V  ◄─────────────────────────────┘
LM2596 OUT− ───────────► ESP32 GND
```

### Critical Checks Before Power-On
1. **LM2596 output = 5.0V** (measure with multimeter, no load)
2. **L298N 5V jumper REMOVED** — don't use its onboard regulator at 7.4V input
3. **All grounds common** — Battery −, LM2596 OUT−, L298N GND, ESP32 GND tied together
4. **Motor wires** → L298N OUT1/2 (left), OUT3/4 (right)
5. **Battery monitor divider** — 100k + 100k on GPIO 34 (see `pinout.h`)

---

## Pinout (ESP32 → L298N)

| Define | GPIO | L298N Pin | Purpose |
|---|---|---|---|
| `LEFT_IN1` | 26 | IN1 | Left motor direction A |
| `LEFT_IN2` | 27 | IN2 | Left motor direction B |
| `LEFT_EN` | 14 | ENA | Left motor PWM |
| `RIGHT_IN1` | 25 | IN3 | Right motor direction A |
| `RIGHT_IN2` | 33 | IN4 | Right motor direction B |
| `RIGHT_EN` | 32 | ENB | Right motor PWM |
| `BATTERY_ADC_PIN` | 34 | — | Battery voltage divider (ADC1) |

All pin definitions in `pinout.h` — change here, updates everywhere.

---

## Project Structure

```
main/
├── main.ino      # Entry point — setup() and loop()
├── drive.ino     # Motor control, battery monitor, safety features
└── pinout.h      # Pin definitions + ESP32 GPIO reference
```

---

## Key Features

| Feature | Description |
|---|---|
| **Safe stop (coast)** | `stop()` uses LOW/LOW — no H-bridge shoot-through |
| **Dead-time** | 5µs PWM=0 before direction change — prevents shoot-through |
| **Acceleration ramping** | Slew-rate limiter (5%/loop) — protects gears, reduces wheel slip |
| **Progressive steering** | 1.5-power curve — gentle at low stick, full at extremes |
| **Motor PWM limit** | `MAX_MOTOR_PWM = 80` — caps effective voltage to ~6V for 3–6V motors |
| **Battery monitor** | Reads 2S voltage via divider on GPIO 34, emergency stop at 6.0V |
| **millis() rollover safe** | Dance state machine handles 49-day wrap |

---

## Functions Reference

### `driveBegin()`
Call once in `setup()`. Initializes pins, PWM channels, starts stopped.

```cpp
void setup() {
  Serial.begin(115200);
  driveBegin();
}
```

### `drive(int forwardBack, int leftRight)`
Main movement. Both params **0–100**, **50 = center**.

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

**Non-blocking** — motors hold last command until new one.

### `stop()`
Safe coast stop (LOW/LOW on both IN pins). Use `drive(50, 50)` for same effect.

### `checkBattery()`
**Call every `loop()` iteration.** Returns `true` if battery OK, `false` if critical (triggers `stop()`).

```cpp
void loop() {
  if (!checkBattery()) return;  // auto-stop on low battery
  // your code here
}
```

### `getBatteryVoltage()`
Returns last measured pack voltage (float, volts).

### `isBatteryLow()`
Returns `true` if battery below cutoff (with hysteresis).

### `dance(int pause, int spd, int repeats)`
Non-blocking demo sequence. Call once to start, then `danceUpdate()` in `loop()`.

```cpp
void setup() {
  driveBegin();
  dance(1000, 100, 3);  // 1s pauses, full speed, 3 repetitions
}

void loop() {
  if (!checkBattery()) return;
  danceUpdate();
}
```

---

## Configuration (in `drive.ino`)

```cpp
#define MAX_MOTOR_PWM       80   // 0–100, limits motor voltage (80 ≈ 6V on 7.4V)
#define MAX_ACCEL_PER_LOOP  5    // 0–100, acceleration slew rate per loop
#define BATTERY_CUTOFF_V    6.0f // 2S Li-ion minimum safe voltage
#define BATTERY_DIVIDER_RATIO 2.0f // 100k/100k divider = /2
```

---

## Setup (Arduino IDE)

1. Install [Arduino IDE](https://www.arduino.cc/en/software)
2. Add ESP32 board support:
   - File → Preferences → Additional Boards Manager URLs:
     ```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```
   - Tools → Board → Boards Manager → search **esp32** → install **esp32 by Espressif Systems**
3. Connect ESP32 via USB
4. Tools → Board → ESP32 Arduino → **ESP32 Dev Module**
5. Tools → Port → select your COM port
6. Open `main/main.ino`
7. Click **Upload**

---

## Troubleshooting

| Problem | Solution |
|---|---|
| Motors don't spin | Check L298N power (+12V/GND), ENA/ENB jumpers ON, wiring |
| One wheel backward | Swap IN1/IN2 (or IN3/IN4) for that motor in `pinout.h` |
| ESP32 resets randomly | Power issue — verify LM2596 at 5.0V, common ground, battery charged |
| Motors jitter at stop | Normal for some L298N — `stop()` coast minimizes this |
| Battery reads wrong | Verify divider (100k/100k), measure at GPIO 34 with multimeter |
| Upload fails | Hold BOOT button during upload, check COM port |
| `analogRead` fails with Wi-Fi | Use ADC1 pins only (GPIO 34/35/36/39) — `pinout.h` uses 34 |

---

## ESP32 GPIO Quick Reference

### Safe (always)
```
GPIO 13, 16, 17, 18, 19, 21, 22, 23, 32, 33
```

### ADC2 (no `analogRead` with Wi-Fi)
```
GPIO 4, 14, 25, 26, 27
```

### Input-only (no output, no pull-up/down)
```
GPIO 34, 35, 36 (VP), 39 (VN)
```

### Do Not Use (SPI flash)
```
GPIO 6, 7, 8, 9, 10, 11
```

### Boot/Strapping (affect boot — avoid)
```
GPIO 0  — Boot button (HIGH to run)
GPIO 2  — Onboard LED, **Wi-Fi** (LOW at boot)
GPIO 5  — HIGH at boot
GPIO 12 — **Wi-Fi**, LOW at boot (HIGH damages flash)
GPIO 15 — **Wi-Fi**, HIGH at boot
```

### Serial (leave free)
```
GPIO 1 (TX0), GPIO 3 (RX0)
```

---

## License

MIT