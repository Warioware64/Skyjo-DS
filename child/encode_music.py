#!/usr/bin/env python3

# Encodes the game's music track for the DS Download Play child binary.
#
# The child has no filesystem, so its music is linked into the ARM9 binary and
# every byte of it crosses the air at roughly 90 KB/s. The cart version's
# gameMusic.wav is 121 s of 11025 Hz 16-bit stereo PCM -- 5,347,708 bytes, more
# than thirteen times the size of the entire child ROM. It has to shrink, and
# the BIOS compressors are not the way to do it:
#
#   8-bit mono PCM, 520 KB of this very track:
#     wf-nnpack-lzss -ewo      66.7%
#     wf-nnpack-huffman -e8    75.2%
#     wf-nnpack-rle -e         98.0%
#
# Two thirds of a megabyte, and worse, a compressed archive entry makes
# AssetDevice's AssetOpen() malloc and decompress the *whole* file at fopen(),
# because there is no seeking into an LZ77 stream. A compressed track would cost
# most of a megabyte of heap the instant the party starts.
#
# 4-bit IMA-ADPCM is smaller than all of that (485 KB), needs no decompression
# buffer at all -- the player reads nibbles straight out of the linked archive --
# and is seekable by construction. Against the alternative it displaces, plain
# 8-bit PCM at the same rate, it is exactly half the size for 2.4 dB less SNR
# (32.3 vs 34.7 dB measured on a 40 s excerpt), and ADPCM's quantisation noise
# scales with the signal, so it hides under the music instead of hissing through
# the quiet passages. Compressing the ADPCM afterwards only inflates it (LZSS
# reaches 104.7%), which is the expected sign that the redundancy is gone.
#
# ffmpeg does the downmix and the resample. That part is not busywork: dropping
# 11025 Hz to 8000 Hz without a proper low-pass folds everything between 4 and
# 5.5 kHz back down into the midrange, and a naive linear-interpolation resampler
# in this script would do exactly that. The ADPCM encoding is then plain Python,
# since it is trivial arithmetic and the container is ours.
#
# Usage: python encode_music.py <in.wav> <out.ima> [--rate HZ]

import os
import struct
import subprocess
import sys

MAGIC = 0x50444153  # 'SADP' little-endian: "Skyjo ADPcm"
VERSION = 1
DEFAULT_RATE = 8000

# The container. 16 bytes so the nibble stream starts word-aligned, matching
# pack_assets.py's 4-byte payload alignment.
#
#   0   u32  magic 'SADP'
#   4   u16  version
#   6   u16  channels
#   8   u32  sample rate
#   12  u32  sample count
#   16  ...  nibbles, low nibble of each byte first
#
# There are deliberately no per-block state headers. The decoder is bit-exact
# against this encoder, and the only seek the player ever performs is back to
# sample 0 at the loop point, where predictor and step index reset to zero.
HEADER_SIZE = 16

# The IMA-ADPCM tables, as in the IMA/DVI specification. STEP_TABLE is indexed by
# the adaptive step index (0..88); INDEX_TABLE says how each 4-bit code moves
# that index.
STEP_TABLE = [
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41,
    45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209,
    230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876,
    963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749,
    3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630,
    9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623,
    27086, 29794, 32767,
]

INDEX_TABLE = [-1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8]


def decode_pcm(path, rate):
    """Returns the track as mono signed 16-bit PCM at `rate`, via ffmpeg."""
    cmd = ['ffmpeg', '-v', 'error', '-i', path,
           '-ac', '1', '-ar', str(rate), '-f', 's16le', '-']
    try:
        result = subprocess.run(cmd, check=True, stdout=subprocess.PIPE)
    except FileNotFoundError:
        raise SystemExit(
            'encode_music.py needs ffmpeg to downmix and resample the track, '
            'and it is not on PATH.\n'
            'Install it (apt install ffmpeg) and rebuild.')
    except subprocess.CalledProcessError as e:
        raise SystemExit(f'ffmpeg failed on {path} (exit {e.returncode})')

    if not result.stdout:
        raise SystemExit(f'ffmpeg produced no audio from {path}')
    return result.stdout


def encode_ima(pcm):
    """4-bit IMA-ADPCM, two samples per byte, low nibble first.

    Starts from predictor 0 / step index 0, which is what the runtime decoder
    resets to at every loop point."""
    samples = memoryview(pcm).cast('h')
    out = bytearray((len(samples) + 1) // 2)

    predictor = 0
    index = 0
    pending = None
    pos = 0

    step_table = STEP_TABLE  # local lookups, this loop runs ~1M times
    index_table = INDEX_TABLE

    for sample in samples:
        step = step_table[index]
        delta = sample - predictor

        code = 0
        if delta < 0:
            code = 8
            delta = -delta

        # Three successive halvings of the step: the classic IMA quantiser.
        diff = step >> 3
        if delta >= step:
            code |= 4
            delta -= step
            diff += step
        if delta >= (step >> 1):
            code |= 2
            delta -= step >> 1
            diff += step >> 1
        if delta >= (step >> 2):
            code |= 1
            diff += step >> 2

        # Track the decoder exactly, clamps included, or the two drift apart.
        predictor = predictor - diff if code & 8 else predictor + diff
        if predictor < -32768:
            predictor = -32768
        elif predictor > 32767:
            predictor = 32767

        index += index_table[code]
        if index < 0:
            index = 0
        elif index > 88:
            index = 88

        if pending is None:
            pending = code
        else:
            out[pos] = pending | (code << 4)
            pos += 1
            pending = None

    if pending is not None:
        out[pos] = pending

    return bytes(out), len(samples)


def stamp_for(src, rate):
    """What the cached output was built from. The encode takes ~10 s of pure
    Python, so it is skipped whenever this still matches."""
    st = os.stat(src)
    return f'v{VERSION} rate={rate} src={os.path.abspath(src)} ' \
           f'size={st.st_size} mtime={int(st.st_mtime)}\n'


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    rate = DEFAULT_RATE
    for a in sys.argv[1:]:
        if a.startswith('--rate='):
            rate = int(a.split('=', 1)[1])

    if len(args) != 2:
        print('usage: encode_music.py <in.wav> <out.ima> [--rate=HZ]',
              file=sys.stderr)
        return 1

    src, dst = args
    if not os.path.exists(src):
        print(f'encode_music.py: {src} does not exist', file=sys.stderr)
        return 1

    stamp_path = dst + '.stamp'
    stamp = stamp_for(src, rate)
    if os.path.exists(dst) and os.path.exists(stamp_path):
        with open(stamp_path, 'r') as f:
            if f.read() == stamp:
                print(f'  music {os.path.getsize(dst):,} bytes (cached)')
                return 0

    pcm = decode_pcm(src, rate)
    nibbles, count = encode_ima(pcm)

    header = struct.pack('<IHHII', MAGIC, VERSION, 1, rate, count)

    os.makedirs(os.path.dirname(os.path.abspath(dst)), exist_ok=True)
    with open(dst, 'wb') as f:
        f.write(header)
        f.write(nibbles)
    with open(stamp_path, 'w') as f:
        f.write(stamp)

    raw = os.path.getsize(src)
    total = HEADER_SIZE + len(nibbles)
    print(f'  music {os.path.basename(src)}: {raw:,} -> {total:,} bytes '
          f'({count:,} samples, {count / rate:.0f}s mono @ {rate} Hz, '
          f'{100.0 * total / raw:.0f}%)')
    return 0


if __name__ == '__main__':
    sys.exit(main())
