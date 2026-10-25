#!/usr/bin/env python3
"""
WAV -> C header converter for ESP32 I2S playback.

Expects a 16-bit mono WAV and emits a const int16_t array plus size/sample-rate
macros. No external deps (stdlib only).

Usage:
  python3 wav_to_header.py <in.wav> [out.h] [array_name]

To make a suitable WAV from any audio (macOS, no ffmpeg needed):
  afconvert -f WAVE -d LEI16@16000 -c 1 <in> <out.wav>
"""

import sys
import wave
import struct
from pathlib import Path


def wav_to_header(wav_path, out_path, array_name):
    with wave.open(wav_path, "rb") as w:
        if w.getsampwidth() != 2:
            raise SystemExit("ERROR: expected 16-bit PCM (getsampwidth=2)")
        if w.getnchannels() != 1:
            raise SystemExit("ERROR: expected mono (nchannels=1)")
        rate = w.getframerate()
        n = w.getnframes()
        raw = w.readframes(n)

    samples = struct.unpack(f"<{n}h", raw)

    with open(out_path, "w") as f:
        f.write(f"// Generated from {Path(wav_path).name}\n")
        f.write(f"// {n} samples @ {rate} Hz mono 16-bit\n\n")
        f.write(f"const int16_t {array_name}[] = {{\n")
        for i in range(0, n, 16):
            chunk = samples[i:i + 16]
            f.write("  " + ", ".join(str(s) for s in chunk) + ",\n")
        f.write("};\n\n")
        f.write(f"const uint32_t {array_name}_SIZE = {n};\n")
        f.write(f"const uint32_t {array_name}_SAMPLE_RATE = {rate};\n")

    print(f"Wrote {out_path}: {n} samples ({n*2} bytes), {n/rate:.2f}s @ {rate} Hz")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)
    wav_path = sys.argv[1]
    out_path = sys.argv[2] if len(sys.argv) > 2 else str(Path(wav_path).with_suffix(".h"))
    array_name = sys.argv[3] if len(sys.argv) > 3 else Path(wav_path).stem
    wav_to_header(wav_path, out_path, array_name)
