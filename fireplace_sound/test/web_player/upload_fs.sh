#!/usr/bin/env bash
set -euo pipefail

# Build a LittleFS image from ../sounds/*.mp3 and flash it into the spiffs
# partition (offset 0x610000, size 0x9E0000) defined in partitions.csv.
#
# Usage:
#   upload_fs.sh            # auto-detect the USB port
#   upload_fs.sh <port>     # explicit port

BASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SOUNDS_DIR="$BASE_DIR/../../sounds"
MKLITTLEFS="$HOME/Library/Arduino15/packages/esp32/tools/mklittlefs/4.0.2-db0513a/mklittlefs"
ESPTOOL="$HOME/Library/Arduino15/packages/esp32/tools/esptool_py/5.3.1/esptool"
SPIFFS_OFFSET=0x610000
SPIFFS_BYTES=10354688   # 0x9E0000

PORT="${1:-}"
if [ -z "$PORT" ]; then
  PORT="$(ls /dev/cu.usbmodem* 2>/dev/null | head -1)"
fi
if [ -z "$PORT" ]; then
  echo "ERROR: no ESP32 device found (/dev/cu.usbmodem* empty)" >&2
  exit 1
fi

if [ ! -d "$SOUNDS_DIR" ]; then
  echo "ERROR: sounds dir not found: $SOUNDS_DIR" >&2
  exit 1
fi

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

echo "Copying sounds from $SOUNDS_DIR ..."
cp "$SOUNDS_DIR"/*.mp3 "$WORK"/

echo "Building LittleFS image ($SPIFFS_BYTES bytes) ..."
"$MKLITTLEFS" -c "$WORK" -p 256 -b 4096 -s "$SPIFFS_BYTES" "$WORK/littlefs.bin"

echo "Flashing LittleFS to $PORT @ $SPIFFS_OFFSET ..."
"$ESPTOOL" --chip esp32s3 --port "$PORT" write_flash "$SPIFFS_OFFSET" "$WORK/littlefs.bin"

echo "Done. Files flashed:"
ls -1 "$SOUNDS_DIR"/*.mp3 | sed 's#.*/##'
