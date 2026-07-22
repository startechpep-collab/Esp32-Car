#pragma once
// ================== pinout.h ==================
// Central place for every GPIO the car uses on an ESP32-DevKitC V2 (WROOM-32).
// Change a number here and it updates everywhere.

// ---- L298N motor driver (2WD) ----
#define LEFT_IN1   26   // -> L298N IN1  (left motor direction A)
#define LEFT_IN2   27   // -> L298N IN2  (left motor direction B)
#define LEFT_EN    14   // -> L298N ENA  (left motor speed / PWM)

#define RIGHT_IN1  25   // -> L298N IN3  (right motor direction A)
#define RIGHT_IN2  33   // -> L298N IN4  (right motor direction B)
#define RIGHT_EN   32   // -> L298N ENB  (right motor speed / PWM)


// =====================================================================
//  ESP32-DevKitC V2 (WROOM-32) PIN REFERENCE  (read before picking new pins)
// =====================================================================
//
// SAFE general-purpose pins (input + output, no special behavior):
//   GPIO 4, 13, 14, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33
//
// INPUT-ONLY pins (no output, NO internal pull-up/down) - never for motors:
//   GPIO 34, 35, 36 (VP), 39 (VN)
//
// DO NOT USE - wired to the internal SPI flash (will crash the chip):
//   GPIO 6, 7, 8, 9, 10, 11
//
// BOOT / STRAPPING pins (usable, but affect boot - avoid if possible,
// keep them at their required level during power-up):
//   GPIO 0  -> must be HIGH to run (LOW = flash mode). Has a boot button.
//   GPIO 2  -> must be LOW/floating at boot. Onboard LED on many boards.
//   GPIO 5  -> must be HIGH at boot.
//   GPIO 12 (MTDI) -> must be LOW at boot (HIGH can brick flash voltage).
//   GPIO 15 (MTDO) -> must be HIGH at boot.
//
// SERIAL / programming (leave free unless you know what you're doing):
//   GPIO 1  (TX0), GPIO 3 (RX0) - used by USB serial upload & Serial.
//
// ADC2 pins (2, 4, 12-15, 25-27) cannot read analog while Wi-Fi is on.
// =====================================================================
