#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>

// --- Configuration ---
const char* ssid = "Galaxy S23l";
const char* password = "leroybarnie";
const String serverUrl = "http://10.13.88.224:5002/api"; 

// --- Sensor Settings ---
#define LIGHT_PIN 9 // Ensure your LDR voltage divider is connected to GPIO 34
#define ADC_MAX 4095.0 

unsigned long lastUpdate = 0;
const long interval = 3000; // 3-second interval complies with the 2-5s requirement

// --- Function to read and smooth light levels ---
float readLightLevel() {
  int totalADC = 0;
  int samples = 50; // Oversampling for a smoother average reading

  for (int i = 0; i < samples; i++) {
    totalADC += analogRead(LIGHT_PIN);
    delay(2); 
  }
  
  float avgADC = (float)totalADC / samples;
  
  // Convert raw ADC (0-4095) to a percentage (0-100%)
  // Note: Depending on if your LDR is tied to ground or 3.3V, 
  // you might need to invert this by doing: 100.0 - ((avgADC / ADC_MAX) * 100.0)
  float lightPercentage = (avgADC / ADC_MAX) * 100.0;
  Serial.printf("Raw ADC: %.2f, Light Level: %.2f%%\n", avgADC, lightPercentage);
  return lightPercentage;
}

void setup() {
  Serial.begin(115200);

  // Configure ADC for 12-bit resolution
  analogReadResolution(12);
  analogSetPinAttenuation(LIGHT_PIN, ADC_11db); 
  
  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nConnected! IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  unsigned long currentMillis = millis();

  // Sends data every 3 seconds to comply with the 2-5 second requirement
  if (currentMillis - lastUpdate >= interval) {
    if (WiFi.status() == WL_CONNECTED) {
      
      float currentLight = readLightLevel();
      Serial.printf("Current Light Level: %.2f%%\n", currentLight);

      HTTPClient http;
      
      // 1. Send ACTUAL sensor data to Flask server
      http.begin(serverUrl + "/data");
      http.addHeader("Content-Type", "application/json");
      
      // Changed the JSON key from "temperature" to "light"
      String payload = "{\"light\":" + String(currentLight, 2) + "}"; 
      
      int httpResponseCode = http.POST(payload);
      Serial.printf("POST Response: %d\n", httpResponseCode);
      http.end();

      // 2. Fetch pattern
      http.begin(serverUrl + "/pattern");
      int httpCode = http.GET();
      if (httpCode > 0) {
        String serverPattern = http.getString();
        Serial.printf("Server Pattern: %s\n", serverPattern.c_str());
      }
      http.end();
    }
    lastUpdate = currentMillis;
  }
}