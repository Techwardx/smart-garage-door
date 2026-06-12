#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>

// Replace with your network credentials
const char* ssid = "Horus@HOME";
const char* password = "dhzy1308";

// Device info for Google Home
const char* deviceName = "ESP32-Sensor";
const char* deviceType = "hall sensor";

WebServer server(80);

// Sensor state (you'll replace this with your actual sensor reading)
bool sensorState = false; // false = off, true = on

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Connect to WiFi
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Signal strength (RSSI): ");
  Serial.println(WiFi.RSSI());
  
  // Start mDNS for device discovery
  if (MDNS.begin(deviceName)) {
    Serial.println("mDNS responder started");
    Serial.print("Device name: ");
    Serial.print(deviceName);
    Serial.println(".local");
    
    // Advertise HTTP service
    MDNS.addService("http", "tcp", 80);
  }
  
  // Setup web server endpoints
  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.on("/sensor", handleSensor);
  
  server.begin();
  Serial.println("HTTP server started");
  Serial.println("Ready for Google Home integration!");
}

void handleRoot() {
  String html = "<html><body>";
  html += "<h1>ESP32 Sensor Device</h1>";
  html += "<p>Device: " + String(deviceName) + "</p>";
  html += "<p>Type: " + String(deviceType) + "</p>";
  html += "<p>Current State: " + String(sensorState ? "ON" : "OFF") + "</p>";
  html += "<p><a href='/sensor'>Get Sensor Data (JSON)</a></p>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

void handleStatus() {
  String json = "{";
  json += "\"device\":\"" + String(deviceName) + "\",";
  json += "\"type\":\"" + String(deviceType) + "\",";
  json += "\"status\":\"online\",";
  json += "\"ip\":\"" + WiFi.localIP().toString() + "\"";
  json += "}";
  
  server.send(200, "application/json", json);
}

void handleSensor() {
  // TODO: Replace with actual sensor reading
  // Example: sensorState = digitalRead(SENSOR_PIN);
  sensorState = random(0, 2); // Placeholder: randomly returns true or false
  
  String json = "{";
  json += "\"sensor\":\"" + String(deviceName) + "\",";
  json += "\"state\":\"" + String(sensorState ? "on" : "off") + "\",";
  json += "\"value\":" + String(sensorState ? 1 : 0) + ",";
  json += "\"timestamp\":" + String(millis());
  json += "}";
  
  server.send(200, "application/json", json);
}

void loop() {
  server.handleClient();
}
