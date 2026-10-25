/*
 * MAX98357A I2S Beep Test — ESP32-S3
 *
 * Plays a recognizable beep pattern to make audio output obvious:
 *   short-high beep, short-high beep, long-low beep, random-pitch beep.
 * Serial prints a line before each beep so you can correlate log -> sound.
 *
 * Wiring (MAX98357A -> ESP32-S3):
 *   BCLK -> GPIO5
 *   LRC  -> GPIO4
 *   DIN  -> GPIO18
 *   VIN  -> 3.3V (NOT 5V — 5V makes the amp ignore 3.3V logic)
 *   GND  -> GND
 *   SD   -> 3.3V (enable)
 *   GAIN -> floating (default +9 dB)
 */

#include <driver/i2s_std.h>
#include <math.h>
#include <string.h>

#define I2S_BCLK_PIN  5
#define I2S_LRC_PIN   4
#define I2S_DOUT_PIN  18

#define SAMPLE_RATE   16000
#define AMPLITUDE     12000   // peak int16 (max 32767)

static i2s_chan_handle_t tx_chan = NULL;

void write_samples(const int16_t *samples, size_t n) {
  size_t written = 0;
  i2s_channel_write(tx_chan, samples, n * sizeof(int16_t), &written, portMAX_DELAY);
}

void play_tone(float freq, uint32_t duration_ms) {
  uint32_t total = (uint32_t)((uint64_t)SAMPLE_RATE * duration_ms / 1000);
  const int N = 256;
  int16_t samples[N];
  float phase = 0.0f;
  const float inc = 2.0f * (float)M_PI * freq / (float)SAMPLE_RATE;

  uint32_t remaining = total;
  while (remaining > 0) {
    int n = (remaining < (uint32_t)N) ? (int)remaining : N;
    for (int i = 0; i < n; i++) {
      samples[i] = (int16_t)(sinf(phase) * AMPLITUDE);
      phase += inc;
      if (phase >= 2.0f * (float)M_PI) phase -= 2.0f * (float)M_PI;
    }
    write_samples(samples, n);
    remaining -= n;
  }
}

void play_silence(uint32_t duration_ms) {
  uint32_t total = (uint32_t)((uint64_t)SAMPLE_RATE * duration_ms / 1000);
  const int N = 256;
  int16_t zeros[N];
  memset(zeros, 0, sizeof(zeros));

  uint32_t remaining = total;
  while (remaining > 0) {
    int n = (remaining < (uint32_t)N) ? (int)remaining : N;
    write_samples(zeros, n);
    remaining -= n;
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("\nMAX98357A I2S beep test");
  Serial.printf("Pins: BCLK=%d LRC=%d DIN=%d\n", I2S_BCLK_PIN, I2S_LRC_PIN, I2S_DOUT_PIN);

  i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
  if (i2s_new_channel(&chan_cfg, &tx_chan, NULL) != ESP_OK) {
    Serial.println("FAILED: i2s_new_channel");
    while (1) delay(1000);
  }

  i2s_std_config_t std_cfg = {
    .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
    .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
    .gpio_cfg = {
      .mclk = I2S_GPIO_UNUSED,
      .bclk = (gpio_num_t)I2S_BCLK_PIN,
      .ws   = (gpio_num_t)I2S_LRC_PIN,
      .dout = (gpio_num_t)I2S_DOUT_PIN,
      .din  = I2S_GPIO_UNUSED,
      .invert_flags = {
        .mclk_inv = false,
        .bclk_inv = false,
        .ws_inv   = false,
      },
    },
  };

  if (i2s_channel_init_std_mode(tx_chan, &std_cfg) != ESP_OK) {
    Serial.println("FAILED: i2s_channel_init_std_mode");
    while (1) delay(1000);
  }

  if (i2s_channel_enable(tx_chan) != ESP_OK) {
    Serial.println("FAILED: i2s_channel_enable");
    while (1) delay(1000);
  }

  randomSeed(esp_random());
  Serial.println("I2S started — beeping...");
}

void loop() {
  Serial.println("beep: short high (880 Hz)");
  play_tone(880.0f, 150);
  play_silence(120);

  Serial.println("beep: short high (880 Hz)");
  play_tone(880.0f, 150);
  play_silence(120);

  Serial.println("beep: LONG LOW (440 Hz)");
  play_tone(440.0f, 700);
  play_silence(500);

  int random_hz = random(300, 1201);
  Serial.printf("beep: random pitch (%d Hz)\n", random_hz);
  play_tone((float)random_hz, 300);
  play_silence(600);
}
