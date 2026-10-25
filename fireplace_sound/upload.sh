#!/usr/bin/env bash
set -euo pipefail

# Reusable ESP32-S3 (N16R8) compile + upload via arduino-cli.
#
# Usage:
#   upload.sh                          # compile+upload the fireplace I2S tone test
#   upload.sh <sketch_dir>             # compile+upload a specific sketch
#   upload.sh <sketch_dir> <port>      # override auto-detected port
#
# FQBN targets the ESP32-S3 N16R8: 16 MB flash, 8 MB OPI PSRAM, USB CDC serial.

FQBN="esp32:esp32:esp32s3:FlashSize=16M,PSRAM=opi,CDCOnBoot=cdc"
BASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SKETCH="${1:-$BASE_DIR/test/i2s_tone_test}"
PORT="${2:-}"

if [ -z "$PORT" ]; then
  PORT="$(ls /dev/cu.usbmodem* 2>/dev/null | head -1)"
fi

if [ -z "$PORT" ]; then
  echo "ERROR: no ESP32 device found (/dev/cu.usbmodem* empty)" >&2
  exit 1
fi

echo "Sketch : $SKETCH"
echo "Port   : $PORT"
echo "FQBN   : $FQBN"

arduino-cli compile --fqbn "$FQBN" "$SKETCH"
arduino-cli upload --fqbn "$FQBN" -p "$PORT" "$SKETCH"

echo "Done. Serial monitor: arduino-cli monitor -p $PORT -c baudrate=115200"
