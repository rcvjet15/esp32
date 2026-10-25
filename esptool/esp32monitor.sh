#!/bin/bash

# ESP32 Serial Monitor Script
# Monitors serial output from ESP32 device

# Help function
show_help() {
    cat << EOF
ESP32 Serial Monitor

USAGE:
    esp32monitor.sh [OPTIONS] <BAUD_RATE>

DESCRIPTION:
    Opens a serial monitor connection to ESP32 device.
    Baud rate must be specified and should match the rate set in your sketch.

ARGUMENTS:
    BAUD_RATE       Serial baud rate (required)
                    Common values: 9600, 115200, 921600

OPTIONS:
    -h, --help      Show this help message
    -p, --port      Specify port (default: auto-detect)

EXAMPLES:
    # Monitor at 921600 baud (most common for ESP32-C6)
    esp32monitor.sh 921600

    # Monitor at 115200 baud
    esp32monitor.sh 115200

    # Monitor specific port at 9600 baud
    esp32monitor.sh --port /dev/cu.usbmodem101 9600

USAGE TIPS:
    - Press Ctrl+A then K to exit (confirm with 'y')
    - Press Ctrl+A then H to toggle logging
    - Make sure baud rate matches Serial.begin() in your sketch

COMMON BAUD RATES:
    9600      - Default Arduino baud rate
    115200    - Fast, commonly used
    921600    - Very fast, good for ESP32-C6

EOF
}

# Parse arguments
PORT=""
BAUD=""

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
        *)
            if [[ "$1" =~ ^[0-9]+$ ]]; then
                BAUD="$1"
            else
                echo "❌ Invalid argument: $1"
                echo ""
                show_help
                exit 1
            fi
            shift
            ;;
    esac
done

# Check if baud rate is provided
if [ -z "$BAUD" ]; then
    echo "❌ Error: Baud rate is required!"
    echo ""
    show_help
    exit 1
fi

# Validate baud rate
if ! [[ "$BAUD" =~ ^[0-9]+$ ]]; then
    echo "❌ Error: Baud rate must be a number"
    echo "Common values: 9600, 115200, 921600"
    exit 1
fi

# Find port if not specified
if [ -z "$PORT" ]; then
    PORT=$(ls /dev/cu.usbmodem* 2>/dev/null | head -1)
    if [ -z "$PORT" ]; then
        # Try other common ESP32 ports
        PORT=$(ls /dev/cu.usbserial* 2>/dev/null | head -1)
    fi
    if [ -z "$PORT" ]; then
        PORT=$(ls /dev/cu.wchusbserial* 2>/dev/null | head -1)
    fi

    if [ -z "$PORT" ]; then
        echo "❌ No ESP32 device found!"
        echo ""
        echo "Available ports:"
        ls /dev/cu.* 2>/dev/null || echo "  None found"
        echo ""
        echo "Please connect your ESP32 and try again."
        echo "Or specify port manually: esp32monitor.sh --port /dev/cu.XXX $BAUD"
        exit 1
    fi
fi

# Check if port exists
if [ ! -e "$PORT" ]; then
    echo "❌ Port does not exist: $PORT"
    echo ""
    echo "Available ports:"
    ls /dev/cu.* 2>/dev/null
    exit 1
fi

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "📺 ESP32 Serial Monitor"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "🔌 Port: $PORT"
echo "⚡ Baud: $BAUD"
echo ""
echo "Controls:"
echo "  Ctrl+A then K - Exit monitor"
echo "  Ctrl+A then H - Toggle logging"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# Check if screen is available
if ! command -v screen &> /dev/null; then
    echo "❌ 'screen' command not found!"
    echo ""
    echo "Please install screen:"
    echo "  brew install screen"
    exit 1
fi

# Start serial monitor
screen "$PORT" "$BAUD"
