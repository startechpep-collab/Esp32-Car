// ================== main.ino ==================
// Uses drive() and stop() from drive.ino.
// Values are only 100 or 50. 50,50 = center/stop.

void setup() {
  Serial.begin(115200);
  driveBegin();
}

void loop() {
  dance(1000);
}
