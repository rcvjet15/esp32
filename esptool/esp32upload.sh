#!/bin/bash

# ESP32-C6 Upload Script
# Uploads compiled Arduino sketches to ESP32-C6 using esptool

# Configuration
ESPTOOL="/Users/rcvjetkovic/Library/Arduino15/packages/esp32/tools/esptool_py/4.6/esptool"
SKETCH_CACHE="/Users/rcvjetkovic/Library/Caches/arduino/sketches"
BOOT_APP0="/Users/rcvjetkovic/Library/Arduino15/packages/esp32/hardware/esp32/3.0.7/tools/partitions/boot_app0.bin"

# Help function
show_help() {
    cat << EOF
ESP32-C6 Upload Script

USAGE:
    esp32upload.sh [OPTIONS] [SKETCH_NAME]

DESCRIPTION:
    Uploads compiled Arduino sketches to ESP32-C6 device.
    If no sketch name is provided, uploads the most recently compiled sketch.

OPTIONS:
    -h, --help      Show this help message
    -p, --port      Specify port (default: auto-detect)
    -b, --baud      Baud rate (default: 921600)

EXAMPLES:
    # Upload most recent sketch
    esp32upload.sh

    # Upload specific sketch by name
    esp32upload.sh esp32tryout

    # Upload with partial name match
    esp32upload.sh tryout

    # Upload to specific port
    esp32upload.sh --port /dev/cu.usbmodem101

    # Upload with different baud rate
    esp32upload.sh --baud 460800 mysketch

WORKFLOW:
    1. Compile in Arduino IDE: Sketch → Export Compiled Binary
    2. Run this script to upload
    3. Monitor output with esp32monitor.sh

TROUBLESHOOTING:
    If upload fails with "Connecting...":
    - Hold the BOOT button on ESP32-C6
    - Run this script
    - Release BOOT when you see "Connecting..."

EOF
}

# Parse arguments
BAUD=921600
PORT=""
SKETCH_FILTER=""

while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            exit 0
            ;;
        -p|--port)
            PORT="$2"
            shift 2
            ;;
        -b|--baud)
            BAUD="$2"
            shift 2
            ;;
        *)
            SKETCH_FILTER="$1"
            shift
            ;;
    esac
done

# Find port if not specified
if [ -z "$PORT" ]; then
    PORT=$(ls /dev/cu.usbmodem* 2>/dev/null | head -1)
    if [ -z "$PORT" ]; then
        echo "❌ No ESP32 device found!"
        echo ""
        echo "Available ports:"
        ls /dev/cu.* 2>/dev/null || echo "  None found"
        echo ""
        echo "Please connect your ESP32-C6 and try again."
        exit 1
    fi
fi

# Find most recent sketch or use provided sketch name
if [ -z "$SKETCH_FILTER" ]; then
    SKETCH_DIR=$(ls -t "$SKETCH_CACHE" 2>/dev/null | head -1)
    if [ -z "$SKETCH_DIR" ]; then
        echo "❌ No compiled sketches found!"
        echo "Please compile a sketch in Arduino IDE first."
        exit 1
    fi
    echo "📁 Using most recent sketch"
else
    # Search for sketch by name
    SKETCH_DIR=$(ls -t "$SKETCH_CACHE" 2>/dev/null | grep -i "$SKETCH_FILTER" | head -1)
    if [ -z "$SKETCH_DIR" ]; then
        echo "❌ Sketch not found: $SKETCH_FILTER"
        echo ""
        echo "Available sketches:"
        ls -t "$SKETCH_CACHE" 2>/dev/null | head -5
        exit 1
    fi
    echo "📁 Found sketch matching: $SKETCH_FILTER"
fi

SKETCH_PATH="$SKETCH_CACHE/$SKETCH_DIR"
cd "$SKETCH_PATH" || exit 1

# Find the sketch name
SKETCH_NAME=$(ls *.ino.bin 2>/dev/null | sed 's/\.ino\.bin//')
if [ -z "$SKETCH_NAME" ]; then
    echo "❌ No compiled binary found in sketch directory"
    exit 1
fi

echo "🔌 Port: $PORT"
echo "⚡ Baud: $BAUD"
echo "📝 Sketch: $SKETCH_NAME"
echo "📂 Path: $SKETCH_PATH"
echo ""
echo "Starting upload..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# Upload
"$ESPTOOL" --chip esp32c6 --port "$PORT" --baud "$BAUD" \
  --before default_reset --after hard_reset write_flash -z \
  --flash_mode keep --flash_freq keep --flash_size keep \
  0x0 "${SKETCH_NAME}.ino.bootloader.bin" \
  0x8000 "${SKETCH_NAME}.ino.partitions.bin" \
  0xe000 "$BOOT_APP0" \
  0x10000 "${SKETCH_NAME}.ino.bin"

if [ $? -eq 0 ]; then
    echo ""
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "✅ Upload successful!"
    echo ""
    echo "To monitor serial output, run:"
    echo "  esp32monitor.sh 921600"
    echo ""
else
    echo ""
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "❌ Upload failed!"
    echo ""
    echo "Troubleshooting:"
    echo "  1. Hold BOOT button on ESP32-C6"
    echo "  2. Run this script again"
    echo "  3. Release BOOT when you see 'Connecting...'"
    echo ""
    exit 1
fi
