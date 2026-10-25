// ESP32 DAC Audio Test - Simple Tone
// This sketch generates a 440Hz tone to test your amplifier and speakers

#define DAC_PIN 25  // GPIO25 = DAC1 on ESP32

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n=================================");
  Serial.println("ESP32 DAC Audio Test - 440Hz Tone");
  Serial.println("=================================");
  Serial.println("You should hear a constant tone from your speakers");
  Serial.println("If you hear distortion, lower the volume on HW327");
  Serial.println("\nWiring:");
  Serial.println("  ESP32 GPIO25 -> HW327 L Input");
  Serial.println("  ESP32 GPIO26 -> HW327 R Input (or duplicate GPIO25)");
  Serial.println("  ESP32 GND    -> HW327 GND");
  Serial.println("  ESP32 VIN    -> HW327 VCC (5V)");
  Serial.println("=================================\n");
}

void loop() {
  // Generate 440Hz sine wave (musical note A)
  // One cycle = 100 samples at ~2272us = 440Hz
  for (int i = 0; i < 100; i++) {
    float angle = (i / 100.0) * 2 * PI;
    uint8_t value = 128 + 127 * sin(angle);
    dacWrite(DAC_PIN, value);
    delayMicroseconds(22);  // ~440Hz timing
  }
}
