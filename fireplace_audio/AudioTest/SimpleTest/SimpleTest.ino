// Simplest ESP32 DAC Audio Test
// Just outputs a constant tone - no libraries, no timers

#define DAC_PIN 25  // GPIO25 = DAC1

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32 Simple Audio Test");
  Serial.println("Wiring:");
  Serial.println("  GPIO25 -> HW327 L and R inputs");
  Serial.println("  GND -> HW327 GND");
  Serial.println("  VIN -> HW327 VCC");
  Serial.println("Playing 440Hz tone...");
}

void loop() {
  // Generate 440Hz sine wave
  for (int i = 0; i < 100; i++) {
    float angle = (i / 100.0) * 2 * PI;
    uint8_t value = 128 + 127 * sin(angle);
    dacWrite(DAC_PIN, value);
    delayMicroseconds(22);  // ~440Hz
  }
}
