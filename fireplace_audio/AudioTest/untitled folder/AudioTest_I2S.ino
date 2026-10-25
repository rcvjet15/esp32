// ESP32 I2S Audio Test - MAX98357A
// This sketch generates a 440Hz tone using I2S output

#include <driver/i2s.h>

// I2S pins for MAX98357A
#define I2S_BCLK      26  // Bit Clock
#define I2S_LRC       25  // Left/Right Clock (Word Select)
#define I2S_DOUT      22  // Data Out

#define SAMPLE_RATE   44100  // 44.1kHz

void setupI2S() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false,
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_BCLK,
    .ws_io_num = I2S_LRC,
    .data_out_num = I2S_DOUT,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n\n========================================");
  Serial.println("ESP32 I2S Audio Test - MAX98357A");
  Serial.println("========================================");
  Serial.println("Generating 440Hz tone...");
  Serial.println("\nWiring:");
  Serial.println("  ESP32 GPIO25 -> MAX98357A LRC");
  Serial.println("  ESP32 GPIO26 -> MAX98357A BCLK");
  Serial.println("  ESP32 GPIO22 -> MAX98357A DIN");
  Serial.println("  ESP32 GND    -> MAX98357A GND");
  Serial.println("  ESP32 VIN    -> MAX98357A VIN (5V)");
  Serial.println("========================================\n");

  setupI2S();
  Serial.println("I2S initialized. Playing tone...");
}

void loop() {
  // Generate 440Hz sine wave
  const int samplesPerCycle = SAMPLE_RATE / 440;  // ~100 samples
  int16_t sample;
  size_t bytes_written;

  for (int i = 0; i < samplesPerCycle; i++) {
    float angle = (i / (float)samplesPerCycle) * 2 * PI;
    sample = (int16_t)(sin(angle) * 10000);  // Amplitude: 10000 (adjust for volume)

    i2s_write(I2S_NUM_0, &sample, sizeof(sample), &bytes_written, portMAX_DELAY);
  }
}
