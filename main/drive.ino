// ================== drive.ino ==================
// 2WD motor driver for an L298N.
// Call driveBegin() once in setup(), then drive(forwardBack, leftRight)
// with values 0..100 (50 = center). Example: drive(70, 70).
//
//   forwardBack: 100 = full forward, 0 = full reverse, 50 = none
//   leftRight:   100 = full right,   0 = full left,    50 = straight
//
// Every call HOLDS until the next call. drive(50, 50) = stop.

#include "pinout.h"

#define JOY_CENTER 50
#define PWM_FREQ 20000  // 20 kHz — above audible range, smooth motor control
#define PWM_RES 8       // 8-bit resolution (0-255)
#define PWM_CH_LEFT 2   // LEDC channel for left motor (avoid 0,1 used by tone/Servo)
#define PWM_CH_RIGHT 3  // LEDC channel for right motor

// Motor PWM limit — protects 3-6V motors from 7.4V battery
// 80 = ~80% duty = ~6V effective on 7.4V supply
#define MAX_MOTOR_PWM 80

// Acceleration ramping (slew-rate limiter)
#define MAX_ACCEL_PER_LOOP 5   // max speed change per loop iteration (0-100 scale)
static int lastLeftSpeed = 0;
static int lastRightSpeed = 0;

// Forward declarations (so main.ino can see them)
void drive(int forwardBack, int leftRight);
void stop();
void dance(int pause, int spd, int repeats);
void danceUpdate();

// Battery monitor state
static uint32_t lastBatteryCheck = 0;
static float batteryVoltage = 0.0f;
static bool batteryLow = false;

// Non-blocking dance state machine
static int d_pause = 0;
static int d_spd = 0;
static uint8_t d_targetRepeats = 0;
static uint8_t d_repeats = 0;
static uint8_t d_state = 0;
static uint32_t d_nextTime = 0;
static bool d_active = false;

void driveBegin() {
  pinMode(LEFT_IN1, OUTPUT);
  pinMode(LEFT_IN2, OUTPUT);
  pinMode(LEFT_EN, OUTPUT);
  pinMode(RIGHT_IN1, OUTPUT);
  pinMode(RIGHT_IN2, OUTPUT);
  pinMode(RIGHT_EN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  ledcSetup(PWM_CH_LEFT, PWM_FREQ, PWM_RES);
  ledcAttachPin(LEFT_EN, PWM_CH_LEFT);
  ledcSetup(PWM_CH_RIGHT, PWM_FREQ, PWM_RES);
  ledcAttachPin(RIGHT_EN, PWM_CH_RIGHT);

  drive(50, 50); // start stopped

  beepBoot();  // startup sound
}

// Set one motor: speed -100..100 (sign = direction)
// en = LEDC channel
// Includes dead-time: sets PWM=0 before changing direction to prevent shoot-through
void setMotor(uint8_t in1, uint8_t in2, uint8_t en, int speed) {
  speed = constrain(speed, -100, 100);

  // Dead-time: ensure PWM is 0 before flipping direction pins
  ledcWrite(en, 0);
  delayMicroseconds(5);  // brief dead-time for H-bridge recovery

  if (speed > 0) {
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
  } else if (speed < 0) {
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
  } else {
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
  }

  ledcWrite(en, map(abs(speed), 0, 100, 0, 255));
}

// Main entry point. drive(70, 70) etc.
// Applies acceleration ramping and progressive steering curve for low inputs
void drive(int forwardBack, int leftRight) {
  forwardBack = constrain(forwardBack, 0, 100);
  leftRight = constrain(leftRight, 0, 100);

  int throttle = forwardBack - JOY_CENTER; // -50..+50
  int steer = leftRight - JOY_CENTER;      // -50..+50

  // Progressive steering curve: low steer inputs give proportionally less turn
  // Normalized 1.5-power curve: (|steer|/50)^1.5 * 50 preserves full range
  float steerNorm = abs(steer) / 50.0;
  float steerProgressiveF = pow(steerNorm, 1.5) * 50.0;
  int steerProgressive = (steer >= 0) ? (int)steerProgressiveF : -(int)steerProgressiveF;

  int leftSpeed = constrain((throttle + steerProgressive) * 2, -100, 100);
  int rightSpeed = constrain((throttle - steerProgressive) * 2, -100, 100);

  // Acceleration ramping (slew-rate limiter)
  leftSpeed = constrain(leftSpeed, lastLeftSpeed - MAX_ACCEL_PER_LOOP, lastLeftSpeed + MAX_ACCEL_PER_LOOP);
  rightSpeed = constrain(rightSpeed, lastRightSpeed - MAX_ACCEL_PER_LOOP, lastRightSpeed + MAX_ACCEL_PER_LOOP);
  lastLeftSpeed = leftSpeed;
  lastRightSpeed = rightSpeed;

  // Apply motor PWM limit (protects motors from overvoltage)
  leftSpeed = constrain(leftSpeed, -MAX_MOTOR_PWM, MAX_MOTOR_PWM);
  rightSpeed = constrain(rightSpeed, -MAX_MOTOR_PWM, MAX_MOTOR_PWM);

  setMotor(LEFT_IN1, LEFT_IN2, PWM_CH_LEFT, leftSpeed);
  setMotor(RIGHT_IN1, RIGHT_IN2, PWM_CH_RIGHT, rightSpeed);
}

// Safe stop: coast (LOW/LOW) — lets motors spin down freely.
// Avoids shorting windings (HIGH/HIGH) which can damage the L298N.
void stop() {
  digitalWrite(LEFT_IN1, LOW);
  digitalWrite(LEFT_IN2, LOW);
  ledcWrite(PWM_CH_LEFT, 0);
  digitalWrite(RIGHT_IN1, LOW);
  digitalWrite(RIGHT_IN2, LOW);
  ledcWrite(PWM_CH_RIGHT, 0);
  lastLeftSpeed = 0;
  lastRightSpeed = 0;
}

// Battery monitor — call periodically from loop()
// Returns true if battery OK, false if critically low (triggers stop)
bool checkBattery() {
  if (millis() - lastBatteryCheck < 1000) return !batteryLow;  // check once/sec
  lastBatteryCheck = millis();

  // Read ADC (12-bit, 0-4095), convert to voltage with divider ratio
  int raw = analogReadMilliVolts(BATTERY_ADC_PIN);
  if (raw < 0) raw = analogRead(BATTERY_ADC_PIN) * 3300 / 4095;  // fallback
  batteryVoltage = (raw / 1000.0f) * BATTERY_DIVIDER_RATIO;

  if (batteryVoltage < BATTERY_CUTOFF_V && !batteryLow) {
    batteryLow = true;
    stop();  // emergency stop
    beepLowBattery();
    Serial.printf("[BATTERY] LOW: %.2fV — STOPPED\n", batteryVoltage);
  } else if (batteryVoltage >= BATTERY_CUTOFF_V + 0.2f) {
    batteryLow = false;  // hysteresis
  }

  return !batteryLow;
}

// Get last measured battery voltage
float getBatteryVoltage() {
  return batteryVoltage;
}

bool isBatteryLow() {
  return batteryLow;
}

// Buzzer helpers (uses LEDC channel 4 for tone)
static bool buzzerInit = false;
void buzzerTone(uint16_t freq, uint16_t durationMs) {
  if (!buzzerInit) {
    ledcSetup(4, freq, 8);
    ledcAttachPin(BUZZER_PIN, 4);
    buzzerInit = true;
  } else {
    ledcWriteTone(4, freq);
  }
  ledcWrite(4, 128);  // 50% duty
  delay(durationMs);
  ledcWrite(4, 0);
}

void beepBoot() {
  buzzerTone(1000, 100);
  delay(50);
  buzzerTone(1500, 100);
  delay(50);
  buzzerTone(2000, 150);
}

void beepLowBattery() {
  for (int i = 0; i < 3; i++) {
    buzzerTone(800, 200);
    delay(200);
  }
}

void beepDanceStart() {
  buzzerTone(1200, 80);
  delay(40);
  buzzerTone(1600, 80);
}

void beepDanceStep() {
  buzzerTone(1800, 40);
}

// Non-blocking dance state machine.
// Call dance(pause, spd, repeats) once to start, then call danceUpdate() in loop().
void dance(int pause, int spd, int repeats) {
  d_pause = pause;
  d_spd = spd;
  d_targetRepeats = repeats;
  d_repeats = 0;
  d_state = 0;
  d_nextTime = 0;
  d_active = true;
  beepDanceStart();
}

void danceUpdate() {
  if (!d_active) return;

  // millis() rollover-safe comparison (handles ~49 day wrap)
  if ((int32_t)(millis() - d_nextTime) < 0) return;

  switch (d_state) {
    case 0: // Forward
      drive(d_spd, 50);
      beepDanceStep();
      d_nextTime = millis() + d_pause;
      d_state = 1;
      break;

    case 1: // Stop after forward
      stop();
      d_nextTime = millis() + d_pause;
      d_state = 2;
      break;

    case 2: // Spin right
      drive(50, 50 + (d_spd + 1) / 2);
      beepDanceStep();
      d_nextTime = millis() + d_pause;
      d_state = 3;
      break;

    case 3: // Stop after spin right
      stop();
      d_nextTime = millis() + d_pause;
      d_state = 4;
      break;

    case 4: // Spin left
      drive(50, 50 - (d_spd + 1) / 2);
      beepDanceStep();
      d_nextTime = millis() + d_pause;
      d_state = 5;
      break;

    case 5: // Stop after spin left
      stop();
      d_nextTime = millis() + d_pause;
      d_state = 6;
      break;

    case 6: // Reverse
      drive(50 - (d_spd + 1) / 2, 50);
      beepDanceStep();
      d_nextTime = millis() + d_pause;
      d_state = 7;
      break;

    case 7: // Stop after reverse
      stop();
      d_repeats++;
      if (d_repeats >= d_targetRepeats) {
        d_active = false;
      } else {
        d_nextTime = millis() + d_pause;
        d_state = 0;
      }
      break;
  }
}