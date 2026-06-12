#include <WiFi.h>

// WiFi set up
const char* ssid = "SSID";
const char* password = "PASSWORD";

// hall hodule set up
const int digitalPin = 5;
const int analogPin = 6;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  // connect to WiFi
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

  // hall module starting
  pinMode(digitalPin, INPUT);
  Serial.println("Hall Sensor: ON");
}

void loop() {
  // put your main code here, to run repeatedly:
  // hall sensor sensing
  int digitalVal = digitalRead(digitalPin);
  int analogVal = analogRead(analogPin);

  Serial.print("Ditigal Status: ");
  Serial.print(digitalVal == LOW ? "Magnet: YES " : "Maget: NO ");
  Serial.print("Amalog: ");
  Serial.println(analogVal);

  delay(1000);
}
