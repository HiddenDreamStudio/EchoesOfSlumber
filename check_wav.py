import wave
import struct
import sys

def check_wav(path):
    try:
        with wave.open(path, 'rb') as f:
            n = f.getnframes()
            data = f.readframes(n)
            fmt = f"{n * f.getnchannels()}h"
            samples = struct.unpack(fmt, data)
            print(f"{path}: max={max(samples)}, min={min(samples)}")
    except Exception as e:
        print(f"Error reading {path}: {e}")

for arg in sys.argv[1:]:
    check_wav(arg)
