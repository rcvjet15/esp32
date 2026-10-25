# Fireplace Sound System — Implementation Plan

ESP32-S3-N16R8 + MAX98357A I2S amplifier, controlled via ESPHome + Home Assistant.

## Architecture

- **MCU:** ESP32-S3-N16R8 (16 MB flash / 8 MB PSRAM), `esp-idf` framework
- **DAC/Amp:** MAX98357A I2S 3W Class-D amplifier driving a 3W 4Ω/8Ω speaker
- **Firmware:** ESPHome exposes an `i2s_audio` media player (`media_player.fireplace_sound`)
- **Audio routing:** Home Assistant streams a mono, low-bitrate loop over Wi-Fi when the fireplace switch turns on

## Pinout (MAX98357A -> ESP32-S3)

| MAX98357A | ESP32-S3   | Function                    |
|-----------|------------|-----------------------------|
| VIN       | 3.3V       | Power input (use 3.3V, NOT 5V) |
| GND       | GND        | Common ground               |
| DIN       | GPIO18     | I2S serial data out         |
| BCLK      | GPIO5      | I2S bit clock               |
| LRC (WS)  | GPIO4      | I2S word select (L/R clock) |
| GAIN      | floating   | +9 dB (default)             |
| SD/MODE   | 3.3V       | Tie high to enable the amp  |
| SPK +/-   | speaker    | 4Ω/8Ω — never connect to GND |

Pin choice avoids S3 strapping pins (GPIO0/3/45/46), USB (GPIO19/20) and flash/PSRAM pins.

> **Power note:** feed VIN from **3.3V**, not 5V. The MAX98357A's logic-high
> threshold scales with its supply; at 5V it needs ~3.5V on BCLK/LRC/DIN, but the
> ESP32-S3 only outputs 3.3V — resulting in silence. At 3.3V supply the amp runs
> within its 2.5–5.5V range and the 3.3V logic levels are reliably detected.

## Files

- `fireplace-sound.yaml` — ESPHome configuration
- `automation.yaml` — Home Assistant automations (ON/OFF)
- `README.md` — wiring, audio hosting, flashing, verification
- `test/i2s_tone_test/i2s_tone_test.ino` — Arduino I2S tone test for hardware bring-up

## Audio preparation

```bash
ffmpeg -i source.mp3 -ac 1 -ar 16000 -b:a 64k fireplace_crackle.mp3
```

Host the file at `/config/www/sounds/fireplace_crackle.mp3` in Home Assistant,
served at `http://<HA_IP>:8123/local/sounds/fireplace_crackle.mp3`.
(If MP3 decoding fails on the media player, fall back to 16-bit mono WAV.)

## Workflow

1. Flash `i2s_tone_test.ino` to confirm the amp/speaker produces sound.
2. Flash `fireplace-sound.yaml` via the ESPHome dashboard or `web.esphome.io`.
3. Adopt `media_player.fireplace_sound` in Home Assistant; test playback + volume slider.
4. Enable the `switch.fireplace_power` automation and verify physical sync.
