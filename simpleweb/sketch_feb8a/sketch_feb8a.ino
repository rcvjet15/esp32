#include <WiFi.h>
#include <WebServer.h>

// Replace with your WiFi credentials
const char* ssid = "HT_910549";
const char* password = "43919240156286395692";

WebServer server(80);

// Current GPIO pin to control
int currentPin = 2;
bool ledState = false;

void handleRoot() {
  String html = "<!DOCTYPE html><html>";
  html += "<head><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>";
  html += "body { font-family: Arial; text-align: center; margin-top: 50px; }";
  html += "button { padding: 20px 40px; font-size: 20px; margin: 10px; cursor: pointer; }";
  html += "input { padding: 10px; font-size: 18px; width: 100px; text-align: center; }";
  html += ".on { background-color: #4CAF50; color: white; border: none; }";
  html += ".off { background-color: #f44336; color: white; border: none; }";
  html += ".set { background-color: #2196F3; color: white; border: none; }";
  html += "</style></head>";
  html += "<body>";
  html += "<h1>ESP32-U GPIO Controller</h1>";
  html += "<p>Current GPIO Pin: <b>" + String(currentPin) + "</b></p>";
  html += "<p>Pin Status: <b>" + String(ledState ? "HIGH" : "LOW") + "</b></p>";
  html += "<hr>";
  html += "<form action='/setpin' method='GET'>";
  html += "<p>Enter GPIO Pin Number: <input type='number' name='pin' value='" + String(currentPin) + "' min='0' max='39'></p>";
  html += "<p><button type='submit' class='set'>Set Pin</button></p>";
  html += "</form>";
  html += "<hr>";
  html += "<p><a href='/led/on'><button class='on'>Turn HIGH</button></a></p>";
  html += "<p><a href='/led/off'><button class='off'>Turn LOW</button></a></p>";
  html += "<p><a href='/'><button>Refresh</button></a></p>";
  html += "</body></html>";

  server.send(200, "text/html", html);
}

void handleSetPin() {
  if (server.hasArg("pin")) {
    int newPin = server.arg("pin").toInt();

    // Valid GPIO pins for ESP32: 0-39 (but avoid some reserved pins)
    if (newPin >= 0 && newPin <= 39) {
      // Reset old pin to input
      pinMode(currentPin, INPUT);

      // Set new pin
      currentPin = newPin;
      pinMode(currentPin, OUTPUT);
      digitalWrite(currentPin, ledState ? HIGH : LOW);

      Serial.print("Pin changed to GPIO");
      Serial.println(currentPin);
    }
  }

  // Redirect back to main page
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleLedOn() {
  ledState = true;
  digitalWrite(currentPin, HIGH);
  Serial.print("GPIO");
  Serial.print(currentPin);
  Serial.println(" turned HIGH");

  // Redirect back to main page
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleLedOff() {
  ledState = false;
  digitalWrite(currentPin, LOW);
  Serial.print("GPIO");
  Serial.print(currentPin);
  Serial.println(" turned LOW");

  // Redirect back to main page
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleNotFound() {
  server.send(404, "text/plain", "404: Not Found");
}

void setup() {
  Serial.begin(115200);
  pinMode(currentPin, OUTPUT);
  digitalWrite(currentPin, LOW);

  Serial.println("\n\nESP32-U GPIO Controller");
  Serial.println("=======================");

  // Connect to WiFi
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n\nWiFi Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Open in browser: http://");
  Serial.println(WiFi.localIP());
  Serial.println("=======================\n");

  // Setup web server routes
  server.on("/", handleRoot);
  server.on("/setpin", handleSetPin);
  server.on("/led/on", handleLedOn);
  server.on("/led/off", handleLedOff);
  server.onNotFound(handleNotFound);

  // Start server
  server.begin();
  Serial.println("Web server started!");
  Serial.print("Default GPIO pin: ");
  Serial.println(currentPin);
}

void loop() {
  server.handleClient();
}
