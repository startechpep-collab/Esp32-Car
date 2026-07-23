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

// Forward declarations (so main.ino can see them)
void drive(int forwardBack, int leftRight);
void stop();
void dance(int pause, int spd, int repeats);
void danceUpdate();

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

  ledcSetup(PWM_CH_LEFT, PWM_FREQ, PWM_RES);
  ledcAttachPin(LEFT_EN, PWM_CH_LEFT);
  ledcSetup(PWM_CH_RIGHT, PWM_FREQ, PWM_RES);
  ledcAttachPin(RIGHT_EN, PWM_CH_RIGHT);

  drive(50, 50); // start stopped
}

// Set one motor: speed -100..100 (sign = direction)
// en = LEDC channel
void setMotor(uint8_t in1, uint8_t in2, uint8_t en, int speed) {
  speed = constrain(speed, -100, 100);

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
void drive(int forwardBack, int leftRight) {
  forwardBack = constrain(forwardBack, 0, 100);
  leftRight = constrain(leftRight, 0, 100);

  int throttle = forwardBack - JOY_CENTER; // -50..+50
  int steer = leftRight - JOY_CENTER;      // -50..+50

  int leftSpeed = constrain((throttle + steer) * 2, -100, 100);
  int rightSpeed = constrain((throttle - steer) * 2, -100, 100);

  setMotor(LEFT_IN1, LEFT_IN2, PWM_CH_LEFT, leftSpeed);
  setMotor(RIGHT_IN1, RIGHT_IN2, PWM_CH_RIGHT, rightSpeed);
}

// Hard brake: both motor IN pins HIGH shorts the windings for instant stop.
void stop() {
  digitalWrite(LEFT_IN1, HIGH);
  digitalWrite(LEFT_IN2, HIGH);
  ledcWrite(PWM_CH_LEFT, 0);
  digitalWrite(RIGHT_IN1, HIGH);
  digitalWrite(RIGHT_IN2, HIGH);
  ledcWrite(PWM_CH_RIGHT, 0);
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
}

void danceUpdate() {
  if (!d_active) return;

  if (millis() < d_nextTime) return;

  switch (d_state) {
    case 0: // Forward
      drive(d_spd, 50);
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