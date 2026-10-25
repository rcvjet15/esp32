// ESP32 DAC Very Loud Test Signal
// Outputs maximum amplitude square wave for testing

#define DAC_PIN 25

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== LOUD TEST SIGNAL ===");
  Serial.println("This outputs maximum amplitude");
  Serial.println("Should be VERY easy to hear");
  Serial.println("Playing 1Hz square wave (slow beep)");
  Serial.println("\nIf you still hear NOTHING:");
  Serial.println("- Check GPIO25 wire is in correct HW327 hole");
  Serial.println("- Check GND is connected");
  Serial.println("- Try both L and R input holes\n");
}

void loop() {
  // 1Hz square wave - very slow, very loud

  Serial.println(">>> LOUD");
  dacWrite(DAC_PIN, 255);  // Maximum
  delay(500);

  Serial.println("<<< quiet");
  dacWrite(DAC_PIN, 0);    // Minimum
  delay(500);
}
