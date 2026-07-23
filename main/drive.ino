// ================== drive.ino ==================
// 2WD motor driver for an L298N.
// Call driveBegin() once in setup(), then drive(forwardBack, leftRight)
// with values 0..100 (50 = center). Example: drive(70, 70).
//
//   forwardBack: 100 = full forward, 0 = full reverse, 50 = none
//   leftRight:   100 = full right,   0 = full left,    50 = straight
//
// Every call HOLDS until the next call. drive(50, 50) = stop.

#include "pinout.h"   // all motor pins live here

#define JOY_CENTER 50
#define PWM_FREQ 20000  // 20 kHz — above audible range, smooth motor control
#define PWM_RES 8       // 8-bit resolution (0-255)

// Forward declarations (so main.ino can see them)
void drive(int forwardBack, int leftRight);
void stop();
void dance(int pause, int spd);

void driveBegin() {
  pinMode(LEFT_IN1, OUTPUT);
  pinMode(LEFT_IN2, OUTPUT);
  pinMode(LEFT_EN, OUTPUT);
  pinMode(RIGHT_IN1, OUTPUT);
  pinMode(RIGHT_IN2, OUTPUT);
  pinMode(RIGHT_EN, OUTPUT);

  ledcSetup(0, PWM_FREQ, PWM_RES);
  ledcAttachPin(LEFT_EN, 0);
  ledcSetup(1, PWM_FREQ, PWM_RES);
  ledcAttachPin(RIGHT_EN, 1);

  drive(50, 50); // start stopped
}

// Set one motor: speed -100..100 (sign = direction)
// en = LEDC channel (0 or 1)
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

  setMotor(LEFT_IN1, LEFT_IN2, 0, leftSpeed);
  setMotor(RIGHT_IN1, RIGHT_IN2, 1, rightSpeed);
}

// Hard brake: both motor IN pins HIGH shorts the windings for instant stop.
void stop() {
  digitalWrite(LEFT_IN1, HIGH);
  digitalWrite(LEFT_IN2, HIGH);
  ledcWrite(0, 255);
  digitalWrite(RIGHT_IN1, HIGH);
  digitalWrite(RIGHT_IN2, HIGH);
  ledcWrite(1, 255);
}

// Dance: moves around and returns to the same spot. Blocking — runs
// the full sequence then stops. pause = ms to stop between moves.
void dance(int pause, int spd) {

  // Forward
  drive(spd, 50);
  delay(pause);
  stop();
  delay(pause);

  // Spin right 360
  drive(50, 50 + spd / 2);
  delay(pause);
  stop();
  delay(pause);

  // Reverse
  drive(50 - spd / 2, 50);
  delay(pause);
  stop();
  delay(pause);

  // Spin left 360
  drive(50, 50 - spd / 2);
  delay(pause);
  stop();
  delay(pause);
}
