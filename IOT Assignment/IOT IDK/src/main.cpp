#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>

// ==================== CONFIGURATION ====================
const char* ssid = "Michael";
const char* password = "messinagoat";
const String serverBaseUrl = "http://172.20.10.4:5002";
const String serverUrl = serverBaseUrl + "/api";

// Sensor
#define LIGHT_PIN 9
#define ADC_MAX 4095.0
unsigned long lastSensorUpdate = 0;
const long sensorInterval = 3000;   // 3 seconds

// LEDs
#define NUM_LEDS 3
const int ledPins[NUM_LEDS] = {6, 10, 12};  // Red, Yellow, Green

// Pattern system
String currentPattern = "blink";
unsigned long lastPatternStep = 0;
int patternStep = 0;
const long patternTick = 150;   // ms per animation step

// ======================================================

float readLightLevel() {
  int totalADC = 0;
  int samples = 50;
  for (int i = 0; i < samples; i++) {
    totalADC += analogRead(LIGHT_PIN);
    delay(2);
  }
 
  float avgADC = (float)totalADC / samples;
  float lightPercentage = 100.0 - ((avgADC / ADC_MAX) * 100.0);
 
  Serial.printf("Raw ADC: %.2f, Light Level: %.2f%%\n", avgADC, lightPercentage);
  return lightPercentage;
}

// ==================== LED HELPERS ====================
void setAllLEDs(bool state) {
  for (int i = 0; i < NUM_LEDS; i++) {
    digitalWrite(ledPins[i], state ? HIGH : LOW);
  }
}

void updateLEDPattern() {
  unsigned long now = millis();
  if (now - lastPatternStep < patternTick) return;
  lastPatternStep = now;

  if (currentPattern == "blink") {
    bool on = (patternStep % 2 == 0);
    setAllLEDs(on);
    patternStep++;

  } else if (currentPattern == "chase") {
    setAllLEDs(false);
    int pos = patternStep % (NUM_LEDS * 2);
    if (pos < NUM_LEDS) {
      digitalWrite(ledPins[pos], HIGH);
    } else {
      digitalWrite(ledPins[(NUM_LEDS * 2 - 1) - pos], HIGH);
    }
    patternStep++;

  } else if (currentPattern == "fire") {
    setAllLEDs(false);
    for (int i = 0; i < NUM_LEDS-1; i++) {
      digitalWrite(ledPins[i], (random(0, 100) > 35) ? HIGH : LOW);
    }

  } else if (currentPattern == "rainbow") {
    setAllLEDs(false);
    int led = patternStep % NUM_LEDS;
    digitalWrite(ledPins[led], HIGH);
    patternStep++;

  } else {
    setAllLEDs(true);  // fallback
  }
}

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);

  // ADC setup for light sensor
  analogReadResolution(12);
  analogSetPinAttenuation(LIGHT_PIN, ADC_11db);

  // LED pins
  for (int i = 0; i < NUM_LEDS; i++) {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], LOW);
  }

  // Wi-Fi
  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connected!");
}

// ==================== LOOP ====================
void loop() {
  unsigned long now = millis();

  // === Sensor + Flask communication every 3 seconds ===
  if (now - lastSensorUpdate >= sensorInterval) {
    if (WiFi.status() == WL_CONNECTED) {
      
      
      Serial.println("\n========================================");
      Serial.print("📍 ESP32 IP Address: ");
      Serial.println(WiFi.localIP());
      Serial.print("🌐 Open browser at: ");
      Serial.println(serverBaseUrl);
      Serial.println("Ready - Sensor + LED patterns active");
      Serial.println("========================================");

      float lightLevel = readLightLevel();

      // Send light data
      HTTPClient http;
      http.begin(serverUrl + "/data");
      http.addHeader("Content-Type", "application/json");
      String payload = "{\"light\":" + String(lightLevel, 1) + "}";
      int code = http.POST(payload);
      Serial.printf("POST /data → %d\n", code);
      http.end();

      // Fetch current pattern from server
      http.begin(serverUrl + "/pattern");
      int getCode = http.GET();
      if (getCode > 0) {
        String newPattern = http.getString();
        newPattern.trim();
        if (newPattern.length() > 0 && newPattern != currentPattern) {
          currentPattern = newPattern;
          patternStep = 0;
          Serial.printf("→ New pattern: %s\n", currentPattern.c_str());
        }
      }
      http.end();
    }
    lastSensorUpdate = now;
  }

  // Run LED animation continuously
  updateLEDPattern();

  delay(10);
}