// ================== web_server.ino ==================
// Minimal ESP32 web server - serves a basic HTML page.

#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

WebServer server(80);

void handleRoot() {
  server.send(200, "text/html", R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head><title>ESP32 Car</title></head>
    <body style="font-family:sans-serif;text-align:center;padding:20px;">
      <h1>ESP32 RC Car</h1>
      <p>WiFi connected. IP: %IP%</p>
      <button onclick="fetch('/forward')">Forward</button>
      <button onclick="fetch('/stop')">Stop</button>
      <button onclick="fetch('/reverse')">Reverse</button>
    </body>
    </html>
  )rawliteral".replace("%IP%", WiFi.localIP().toString()));
}

void handleForward() {
  server.send(200, "text/plain", "forward");
}

void handleStop() {
  server.send(200, "text/plain", "stop");
}

void handleReverse() {
  server.send(200, "text/plain", "reverse");
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("Connected! IP: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/forward", handleForward);
  server.on("/stop", handleStop);
  server.on("/reverse", handleReverse);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}