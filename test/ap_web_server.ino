// ================== ap_web_server.ino ==================
// Minimal ESP32 AP mode web server - fixed IP, shows "ESP32 Car".

#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "ESP32_Car";
const char* password = "12345678";

IPAddress local_ip(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

WebServer server(80);

void handleRoot() {
  server.send(200, "text/plain", "ESP32 Car");
}

void setup() {
  Serial.begin(115200);

  WiFi.softAPConfig(local_ip, gateway, subnet);
  WiFi.softAP(ssid, password[0] ? password : nullptr);

  Serial.print("AP started. IP: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.begin();
}

void loop() {
  server.handleClient();
}