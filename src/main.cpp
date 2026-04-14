#include <Arduino.h>

// Pin definitions
#define LED_BUILTIN 2  // Built-in LED on most ESP32 boards

void setup() {
  // Initialize serial communication
  Serial.begin(115200);

  // Configure LED pin as output
  pinMode(LED_BUILTIN, OUTPUT);

  // Startup message
  Serial.println("Skylink ESP32 Started");
  Serial.println("----------------------");
}

void loop() {
  // Toggle built-in LED
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);

  // Heartbeat message
  Serial.println("Heartbeat: ESP32 is running...");
}
