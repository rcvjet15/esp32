# ESP32 Fireplace Audio Project

## Project Overview
ESP32-based audio player that plays fire crackling sounds through an amplifier and speaker, designed for use with an electric fireplace.

## Hardware Setup

### Components
- **ESP32-U** board
- **HW-327 V0.0.1** amplifier board (based on TPA3110 or similar chip)
- **8Ω 10W speakers** (ideal impedance and power rating)
- **9V battery** (powers the amplifier - USB power alone is insufficient)
- **USB cable** (powers the ESP32)

### Wiring Configuration

**Power connections (HW-327 bottom right - POWER section):**
- 9V Battery (+) → HW-327 VCC
- 9V Battery (-) → HW-327 GND **AND** ESP32 GND (common ground is critical!)

**Audio input (HW-327 top left - INPUT section):**
- ESP32 GPIO25 → HW-327 L (left input)
- ESP32 GPIO25 → HW-327 R (right input) - same signal for mono
- ESP32 GND → HW-327 INPUT GND (audio signal reference)

**Speaker output (HW-327 top right - OUTPUT section):**
- Speaker (+) → HW-327 L+ or R+
- Speaker (-) → HW-327 L- or R-

**USB Power:**
- USB → ESP32 (powers the microcontroller only)

### Key Findings
1. **HW-327 needs external power** - ESP32 USB power (5V/500mA) is insufficient for the amplifier
2. **Common ground is essential** - Battery GND, ESP32 GND, and HW-327 GND must all be connected
3. **9V battery works great** - Provides enough current for clean audio amplification
4. **DAC directly to speaker produces only buzzing** - Amplifier is necessary

## Software Setup

### Arduino IDE Configuration
- **Board**: ESP32 Dev Module
- **ESP32 Arduino Core**: 3.0.7 (uses new timer and DAC API)
- **Upload Speed**: 921600
- **Flash Size**: 4MB
- **Partition Scheme**: Default

### Code Structure

**Working sketches:**
1. **SimpleTest.ino** - Basic 440Hz tone generator (testing)
2. **DiagnosticTest.ino** - Slow 2Hz test for troubleshooting
3. **LoudTest.ino** - Maximum amplitude test signal
4. **AudioSample.ino** - Hardware timer-based sample playback with test beep
5. **FireplaceAudio.ino** - Final working fire sound player ⭐

### Key Code Elements

**ESP32 Core 3.x Timer API:**
```cpp
timer = timerBegin(SAMPLE_RATE);           // Create timer at sample rate
timerAttachInterrupt(timer, &onTimer);     // Attach interrupt function
timerAlarm(timer, 1, true, 0);             // Start alarm (triggers every tick)
```

**ESP32 Core 3.x DAC API:**
```cpp
#include <driver/dac.h>

dac_output_enable(DAC_CHANNEL_1);          // Enable DAC on GPIO25
dac_output_voltage(DAC_CHANNEL_1, value);  // Output 0-255 value
```

## Audio Conversion Process

### Requirements
```bash
pip3 install pydub numpy
brew install ffmpeg
```

### Convert Audio Files
```bash
cd ~/git/iot/esp32/fireplace_audio
python3 convert_audio.py your_audio.mp3 16000 10
```

**Parameters:**
- `16000` = Sample rate in Hz (16kHz is good balance of quality/memory)
- `10` = Duration in seconds (optional, limits file size)

**Memory considerations:**
- 10 seconds @ 16kHz = 160KB
- 20 seconds @ 16kHz = 320KB
- ESP32 has ~320KB usable program space for audio data

### Current Audio File
- **Source**: `613317__solifer__fireplace.mp3` (6MB original)
- **Converted**: 10-second clip @ 16kHz mono 8-bit
- **Output**: `613317__solifer__fireplace_audio.h`
- **Size**: 160,000 bytes (156KB)

## Issues Solved

### 1. No Sound Output
**Problem**: Connected everything but no audio
**Solution**: HW-327 needed external 9V battery power, not just ESP32 USB power

### 2. Distorted Audio
**Problem**: Early tests had timing drift and distortion
**Solution**: Use ESP32 hardware timer instead of `micros()` for precise sample timing

### 3. Compilation Errors with Timer
**Problem**: `timerBegin()` errors with ESP32 core 3.x
**Solution**: New API - `timerBegin(frequency)` instead of `timerBegin(num, prescaler, countUp)`

### 4. dacWrite() Not Declared
**Problem**: `dacWrite()` function not found in core 3.x
**Solution**: Use driver API - `#include <driver/dac.h>` and `dac_output_voltage()`

### 5. Invalid C Variable Names
**Problem**: Audio file starting with number (613317...) created invalid variable names
**Solution**: Python script now prefixes numeric filenames with `audio_`

## Project Files

```
fireplace_audio/
├── PROJECT_NOTES.md                          # This file
├── convert_audio.py                          # Audio conversion script
├── 613317__solifer__fireplace.mp3           # Original audio file
└── AudioTest/
    ├── SimpleTest/
    │   └── SimpleTest.ino                    # Basic 440Hz tone test
    ├── DiagnosticTest/
    │   └── DiagnosticTest.ino               # Slow diagnostic test
    ├── LoudTest/
    │   └── LoudTest.ino                     # Maximum volume test
    ├── AudioSample/
    │   └── AudioSample.ino                  # Test beep with timer
    └── FireplaceAudio/
        ├── FireplaceAudio.ino               # Main working sketch ⭐
        └── 613317__solifer__fireplace_audio.h  # Converted audio data
```

## Next Steps / Improvements

### Potential Enhancements
1. **Longer audio loop** - Convert 20-30 seconds to reduce repetition
2. **Volume control** - Add potentiometer on GPIO for digital volume control
3. **Power switch** - Add button to start/stop playback
4. **Sleep mode** - Put ESP32 in deep sleep to save battery
5. **Multiple sounds** - Store several fire sounds and randomly select
6. **Fade in/out** - Smooth transitions at loop points
7. **External power** - Replace 9V battery with wall adapter for permanent installation
8. **PCB design** - Create custom board with proper connectors

### Audio Quality Improvements
1. **Higher sample rate** - Try 22kHz or 24kHz (but uses more memory)
2. **Better audio source** - Find higher quality fire recording
3. **EQ/filtering** - Pre-process audio to emphasize crackling frequencies
4. **Multiple layers** - Mix multiple fire sounds for more realistic effect

## Testing Checklist

Before uploading new code:
- [ ] Check all ground connections (common ground!)
- [ ] Verify 9V battery has charge
- [ ] Confirm GPIO25 connected to both L and R inputs
- [ ] Speaker properly connected to OUTPUT
- [ ] Serial monitor at 115200 baud for debugging

## Troubleshooting

**No sound:**
1. Check 9V battery voltage
2. Verify common ground connections
3. Test with LoudTest.ino for obvious signal
4. Check speaker connections (try swapping +/-)

**Distorted sound:**
1. Check for loose wire connections
2. Verify battery has sufficient charge
3. Try different GPIO25 connection points

**Won't compile:**
1. Verify ESP32 Arduino core version (3.0.7 tested)
2. Check all #include statements
3. Ensure .h file is in same folder as .ino

**Out of memory:**
1. Reduce audio duration (try 5 seconds)
2. Lower sample rate (try 12kHz)
3. Check partition scheme in Arduino IDE

## Resources

- **ESP32 DAC Documentation**: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/dac.html
- **Hardware Timer Guide**: https://docs.espressif.com/projects/arduino-esp32/en/latest/api/timer.html
- **Free sound effects**: https://freesound.org/
- **PyDub documentation**: https://github.com/jiaaro/pydub

---

**Project Status**: ✅ Working
**Last Updated**: 2026-02-14
**Next Goal**: Test with longer audio clip (20+ seconds)
