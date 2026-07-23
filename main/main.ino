// ================== main.ino ==================
// Uses drive(), stop(), dance(), danceUpdate() from drive.ino.
// Values 0..100 (50 = center/stop).

void setup() {
  Serial.begin(115200);
  driveBegin();
  dance(1000, 100, 3);  // start non-blocking dance
}

void loop() {
  danceUpdate();
}
