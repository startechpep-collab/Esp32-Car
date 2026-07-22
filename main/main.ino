// ================== main.ino ==================
// Uses drive() and stop() from drive.ino.
// Values 0..100 (50 = center/stop).

void setup() {
  Serial.begin(115200);
  driveBegin();
}

void loop() {
  dance(1000);
}
