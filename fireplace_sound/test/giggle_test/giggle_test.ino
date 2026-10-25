/*
 * MAX98357A I2S Playback Test — ESP32-S3
 *
 * Plays a real audio clip (giggle.h, generated from an MP3) in a loop.
 * Use this to verify the full audio chain (MP3 -> PCM -> I2S -> amp -> speaker).
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
#include "giggle.h"

#define I2S_BCLK_PIN  5
#define I2S_LRC_PIN   4
#define I2S_DOUT_PIN  18
#define SAMPLE_RATE   16000

static i2s_chan_handle_t tx_chan = NULL;

void play_audio(const int16_t *data, uint32_t len) {
  const uint32_t CHUNK = 4096;
  uint32_t offset = 0;
  while (offset < len) {
    uint32_t n = (len - offset < CHUNK) ? (len - offset) : CHUNK;
    size_t written = 0;
    i2s_channel_write(tx_chan, data + offset, n * sizeof(int16_t), &written, portMAX_DELAY);
    offset += n;
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("\nMAX98357A I2S playback test");
  Serial.printf("Clip: %u samples @ %u Hz (%.2fs)\n",
                giggle_SIZE, giggle_SAMPLE_RATE, (float)giggle_SIZE / giggle_SAMPLE_RATE);
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

  Serial.println("I2S started.");
}

void loop() {
  Serial.println("playing giggle...");
  play_audio(giggle, giggle_SIZE);
  delay(2000);
}
