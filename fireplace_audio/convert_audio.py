#!/usr/bin/env python3
"""
Audio to Arduino Array Converter
Converts audio files (MP3, WAV, etc.) to C byte arrays for ESP32 DAC playback

Requirements: pip install pydub numpy
Also needs ffmpeg: brew install ffmpeg (on macOS)
"""

import sys
import os
from pathlib import Path

try:
    from pydub import AudioSegment
    import numpy as np
except ImportError:
    print("ERROR: Required libraries not installed")
    print("Please run: pip3 install pydub numpy")
    print("And: brew install ffmpeg (on macOS)")
    sys.exit(1)


def convert_audio_to_array(input_file, output_file=None, sample_rate=16000, duration=None):
    """
    Convert audio file to C byte array for ESP32 DAC

    Args:
        input_file: Path to audio file (MP3, WAV, etc.)
        output_file: Output .h file (optional, defaults to input name + .h)
        sample_rate: Target sample rate in Hz (default 16000)
        duration: Limit to first N seconds (optional)
    """

    print(f"\n{'='*60}")
    print(f"Converting: {input_file}")
    print(f"{'='*60}")

    # Load audio file
    print("Loading audio file...")
    audio = AudioSegment.from_file(input_file)

    # Limit duration if specified
    if duration:
        audio = audio[:duration * 1000]  # pydub works in milliseconds
        print(f"Trimmed to {duration} seconds")

    # Convert to mono
    if audio.channels > 1:
        audio = audio.set_channels(1)
        print("Converted to mono")

    # Resample to target sample rate
    if audio.frame_rate != sample_rate:
        audio = audio.set_frame_rate(sample_rate)
        print(f"Resampled to {sample_rate} Hz")

    # Convert to 8-bit unsigned (DAC format: 0-255, center at 128)
    audio = audio.set_sample_width(1)  # 1 byte = 8 bits

    # Get raw audio data
    raw_data = audio.raw_data
    audio_array = np.frombuffer(raw_data, dtype=np.uint8)

    # Generate output filename
    if output_file is None:
        output_file = Path(input_file).stem + "_audio.h"

    # Generate C header file
    print(f"\nGenerating C array...")
    print(f"  Sample rate: {sample_rate} Hz")
    print(f"  Samples: {len(audio_array)}")
    print(f"  Duration: {len(audio_array) / sample_rate:.2f} seconds")
    print(f"  Memory: {len(audio_array)} bytes")

    with open(output_file, 'w') as f:
        # Write header
        f.write(f"// Audio data generated from {Path(input_file).name}\n")
        f.write(f"// Sample rate: {sample_rate} Hz\n")
        f.write(f"// Duration: {len(audio_array) / sample_rate:.2f} seconds\n")
        f.write(f"// Size: {len(audio_array)} bytes\n\n")

        # Write array declaration
        # Create valid C identifier (can't start with number)
        array_name = Path(input_file).stem.replace('-', '_').replace(' ', '_')
        # If name starts with digit, prefix with 'audio_'
        if array_name[0].isdigit():
            array_name = 'audio_' + array_name
        f.write(f"const uint8_t {array_name}_audio[] = {{\n")

        # Write data in rows of 16 bytes
        for i in range(0, len(audio_array), 16):
            chunk = audio_array[i:i+16]
            hex_values = ', '.join(f'{x:3d}' for x in chunk)
            f.write(f"  {hex_values},\n")

        f.write("};\n\n")
        f.write(f"const int {array_name}_SIZE = sizeof({array_name}_audio);\n")
        f.write(f"const int {array_name}_SAMPLE_RATE = {sample_rate};\n")

    print(f"\nSuccess! Created: {output_file}")
    print(f"\nTo use in Arduino:")
    print(f"  1. Place {output_file} in your sketch folder")
    print(f"  2. Add: #include \"{output_file}\"")
    print(f"  3. Use the array: {array_name}_audio[]")
    print(f"{'='*60}\n")


def main():
    if len(sys.argv) < 2:
        print("Usage: python3 convert_audio.py <audio_file> [sample_rate] [duration]")
        print("\nExamples:")
        print("  python3 convert_audio.py fire.mp3")
        print("  python3 convert_audio.py fire.mp3 16000")
        print("  python3 convert_audio.py fire.mp3 16000 10  # Limit to 10 seconds")
        print("\nSupported formats: MP3, WAV, M4A, FLAC, OGG, etc.")
        sys.exit(1)

    input_file = sys.argv[1]
    sample_rate = int(sys.argv[2]) if len(sys.argv) > 2 else 16000
    duration = int(sys.argv[3]) if len(sys.argv) > 3 else None

    if not os.path.exists(input_file):
        print(f"ERROR: File not found: {input_file}")
        sys.exit(1)

    convert_audio_to_array(input_file, sample_rate=sample_rate, duration=duration)


if __name__ == "__main__":
    main()
