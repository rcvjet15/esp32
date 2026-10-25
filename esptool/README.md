# ESP32-C6 Upload and Monitor Tools

Command-line tools for uploading Arduino sketches and monitoring ESP32-C6 devices.

## 📁 Contents

- `esp32upload.sh` - Upload compiled Arduino sketches to ESP32-C6
- `esp32monitor.sh` - Serial monitor for ESP32 devices

## 🚀 Quick Start

### 1. View Help

```bash
# Upload script help
~/git/iot/esp32/esptool/esp32upload.sh --help

# Monitor script help
~/git/iot/esp32/esptool/esp32monitor.sh --help
```

### 2. Basic Usage

```bash
# Step 1: Compile in Arduino IDE
# Sketch → Export Compiled Binary (Cmd + Alt + S)

# Step 2: Upload to ESP32-C6
~/git/iot/esp32/esptool/esp32upload.sh

# Step 3: Monitor serial output (use your sketch's baud rate)
~/git/iot/esp32/esptool/esp32monitor.sh 921600
```

## 📖 Detailed Usage

### esp32upload.sh

Upload compiled Arduino sketches to ESP32-C6.

**Syntax:**
```bash
esp32upload.sh [OPTIONS] [SKETCH_NAME]
```

**Examples:**
```bash
# Upload most recent sketch
esp32upload.sh

# Upload specific sketch by name
esp32upload.sh esp32tryout

# Upload with partial name match
esp32upload.sh tryout

# Upload to specific port with different baud
esp32upload.sh --port /dev/cu.usbmodem101 --baud 460800
```

**Options:**
- `-h, --help` - Show help message
- `-p, --port <PORT>` - Specify port (default: auto-detect)
- `-b, --baud <RATE>` - Baud rate (default: 921600)

**Troubleshooting Upload:**

If you see "Connecting..." and it hangs:
1. Hold the **BOOT** button on your ESP32-C6
2. Run the upload script
3. Release **BOOT** when you see "Connecting..."

### esp32monitor.sh

Monitor serial output from ESP32 devices.

**Syntax:**
```bash
esp32monitor.sh [OPTIONS] <BAUD_RATE>
```

**Examples:**
```bash
# Monitor at 921600 baud
esp32monitor.sh 921600

# Monitor at 115200 baud
esp32monitor.sh 115200

# Monitor specific port
esp32monitor.sh --port /dev/cu.usbmodem101 9600
```

**Options:**
- `-h, --help` - Show help message
- `-p, --port <PORT>` - Specify port (default: auto-detect)

**Monitor Controls:**
- `Ctrl+A` then `K` - Exit monitor (confirm with 'y')
- `Ctrl+A` then `H` - Toggle logging to file

**Common Baud Rates:**
- `9600` - Default Arduino
- `115200` - Fast, commonly used
- `921600` - Very fast, ESP32-C6 recommended

## 🔧 Setup Aliases (Optional)

For easier access, add aliases to your shell:

```bash
# Add to ~/.zshrc
echo 'export PATH="$HOME/git/iot/esp32/esptool:$PATH"' >> ~/.zshrc
echo 'alias esp32upload="esp32upload.sh"' >> ~/.zshrc
echo 'alias esp32monitor="esp32monitor.sh"' >> ~/.zshrc

# Reload shell configuration
source ~/.zshrc
```

Then you can use short commands:
```bash
esp32upload
esp32monitor 921600
```

## 📋 Complete Workflow Example

```bash
# 1. Write your sketch in Arduino IDE
# 2. Compile: Sketch → Export Compiled Binary (Cmd + Alt + S)
# 3. Upload
esp32upload.sh

# 4. Monitor output (make sure baud matches your Serial.begin())
esp32monitor.sh 921600

# 5. Exit monitor: Ctrl+A then K, then Y
```

## 🐛 Troubleshooting

### Port Not Found

**Check available ports:**
```bash
ls /dev/cu.*
```

**Common ESP32 ports:**
- `/dev/cu.usbmodem*` (USB CDC)
- `/dev/cu.usbserial*` (CP210x)
- `/dev/cu.wchusbserial*` (CH340)

### Upload Fails

1. **Try manual boot mode:**
   - Hold BOOT button
   - Run upload script
   - Release BOOT when "Connecting..." appears

2. **Check USB cable:**
   - Use a data cable (not charging-only)

3. **Try different baud rate:**
   ```bash
   esp32upload.sh --baud 460800
   ```

### Monitor Shows Garbage

- **Wrong baud rate:** Make sure it matches `Serial.begin()` in your sketch
- **Try:** `esp32monitor.sh 115200` or `esp32monitor.sh 9600`

### Permission Denied

```bash
sudo chmod 666 /dev/cu.usbmodem*
```

## 📝 Notes

- Scripts are specifically configured for **ESP32-C6**
- Baud rate in monitor **must match** your sketch's `Serial.begin()`
- Arduino IDE must compile the sketch before uploading
- Upload script auto-detects the most recent compiled sketch

## 🔗 Links

- [ESP32-C6 Datasheet](https://www.espressif.com/en/products/socs/esp32-c6)
- [Arduino ESP32 Documentation](https://docs.espressif.com/projects/arduino-esp32/en/latest/)
- [esptool Documentation](https://docs.espressif.com/projects/esptool/en/latest/)
