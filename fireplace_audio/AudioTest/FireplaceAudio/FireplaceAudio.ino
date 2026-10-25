// ESP32 Fireplace Audio Player
// Plays fire crackling sound in a loop using DAC output

#include <driver/dac.h>
#include "613317__solifer__fireplace_audio.h"

#define DAC_PIN 25
#define DAC_CHANNEL DAC_CHANNEL_1  // GPIO25 = DAC1

// Hardware timer
hw_timer_t *timer = NULL;
volatile int sampleIndex = 0;

// Timer interrupt - called at SAMPLE_RATE frequency
void IRAM_ATTR onTimer() {
  // Output sample to DAC (using direct driver call)
  dac_output_voltage(DAC_CHANNEL, audio_613317__solifer__fireplace_audio[sampleIndex]);

  // Move to next sample
  sampleIndex++;
  if (sampleIndex >= audio_613317__solifer__fireplace_SIZE) {
    sampleIndex = 0;  // Loop the sample
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n\n=======================================");
  Serial.println("ESP32 Fireplace Audio Player");
  Serial.println("=======================================");
  Serial.printf("Sample Rate: %d Hz\n", audio_613317__solifer__fireplace_SAMPLE_RATE);
  Serial.printf("Sample Size: %d bytes\n", audio_613317__solifer__fireplace_SIZE);
  Serial.printf("Duration: %.2f seconds\n", (float)audio_613317__solifer__fireplace_SIZE / audio_613317__solifer__fireplace_SAMPLE_RATE);
  Serial.printf("Memory Usage: %.1f KB\n", audio_613317__solifer__fireplace_SIZE / 1024.0);
  Serial.println("\nPlaying fire crackling sound in loop...");
  Serial.println("\nWiring:");
  Serial.println("  9V Battery + -> HW327 VCC (POWER)");
  Serial.println("  9V Battery - -> HW327 GND + ESP32 GND (common)");
  Serial.println("  ESP32 GPIO25 -> HW327 L and R (INPUT)");
  Serial.println("  Speaker -> HW327 OUTPUT");
  Serial.println("=======================================\n");

  // Enable DAC channel
  dac_output_enable(DAC_CHANNEL);

  // Setup hardware timer (ESP32 core 3.x API)
  timer = timerBegin(audio_613317__solifer__fireplace_SAMPLE_RATE);
  timerAttachInterrupt(timer, &onTimer);
  timerAlarm(timer, 1, true, 0);

  Serial.println("Playing fire sound...");
}

void loop() {
  // Nothing needed here - timer interrupt handles playback
  delay(1000);

  // Optional: Print status every second
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 5000) {
    Serial.printf("Playing... %.1f seconds elapsed\n", millis() / 1000.0);
    lastPrint = millis();
  }
}
