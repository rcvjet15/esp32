# Fireplace Sound System

ESP32-S3-N16R8 + MAX98357A I2S amplifier, streamed by Home Assistant via ESPHome.

## Wiring

| MAX98357A | ESP32-S3   | Function                    |
|-----------|------------|-----------------------------|
| VIN       | 3.3V       | Power input — use 3.3V, NOT 5V |
| GND       | GND        | Common ground               |
| DIN       | GPIO18     | I2S serial data out         |
| BCLK      | GPIO5      | I2S bit clock               |
| LRC (WS)  | GPIO4      | I2S word select (L/R clock) |
| GAIN      | floating   | +9 dB (tie to GND = +3 dB, VIN = +15 dB) |
| SD/MODE   | 3.3V       | Tie high to enable the amp  |
| SPK +/-   | speaker    | 4Ω/8Ω — never connect to GND |

Power the ESP32-S3 via USB; take the amp's VIN from the board's **3.3V** rail.

> **Why 3.3V and not 5V:** the MAX98357A's logic-high threshold scales with its
> supply voltage. At 5V it needs ~3.5V on BCLK/LRC/DIN, but the ESP32-S3 outputs
> only 3.3V, so the amp never registers a valid "high" and stays silent. At 3.3V
> supply the amp runs within its 2.5–5.5V range and reliably detects the logic.

## Audio files

The media player is configured for **WAV** (16-bit mono). Ready-made clips are
in `sounds/`:

- `fireplace_crackle.wav` (60 s)
- `giggle.wav` (4 s test clip)

Copy them into Home Assistant:

```bash
/config/www/sounds/fireplace_crackle.wav
/config/www/sounds/giggle.wav
```

They are served at `http://<HA_IP>:8123/local/sounds/<name>.wav`.

To add a sound, drop a 16-bit mono WAV into `/config/www/sounds/` and add its
name to the `input_select.fireplace_sound` options. Convert any audio with
`afconvert` (macOS, no ffmpeg needed — ffmpeg is broken on this machine):

```bash
afconvert -f WAVE -d LEI16@16000 -c 1 input.mp3 output.wav
```

## Flashing

1. Edit `fireplace-sound.yaml`: set `wifi.ssid`, `wifi.password`.
2. Open the ESPHome dashboard (HA → Settings → Add-ons → ESPHome → Open Web UI).
3. **New device** → name `fireplace-sound` → select **ESP32-S3** → edit → paste the YAML.
4. First flash: connect the S3 via USB and use the ESPHome **web flasher**
   (`https://web.esphome.io`, browser WebSerial) or download the compiled `.bin`
   from the dashboard and flash with esptool.
5. In Home Assistant, **adopt** the device. It appears as `media_player.fireplace_sound`.

## Home Assistant automations

Add the `input_select` and `input_boolean` from the top of `automation.yaml`
into `configuration.yaml`, then paste the automation into `automations.yaml`
(or add via Settings → Automations → YAML edit). Replace `<HA_IP>` with your
Home Assistant host IP.

- `input_select.fireplace_sound` — pick which sound to play (persistent).
- `input_boolean.fireplace_sound_enabled` — the on/off toggle (persistent).

Both live in Home Assistant (always powered), so their state survives the ESP32
resetting or losing power. The control automation plays the selected sound when
`switch.fireplace_power` is on **and** `fireplace_sound_enabled` is on, and stops
otherwise.

## Verification

1. Flash `test/i2s_tone_test/i2s_tone_test.ino` first to confirm the amp + speaker
   output a clean tone (no HA/Wi-Fi required).
2. After flashing ESPHome, play a test clip on `media_player.fireplace_sound` and
   confirm clean audio and a working volume slider.
3. Toggle `switch.fireplace_power` and confirm the sound starts/stops.
