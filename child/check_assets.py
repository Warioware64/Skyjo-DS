#!/usr/bin/env python3

# Checks that the packed archive holds exactly the assets the client build
# loads -- no more, no less.
#
# A missing entry is fatal on the guest: AsyncAssetBatch::Track() terminates
# when the engine refuses a load, and there is no filesystem to fall back on.
# An extra entry is dead weight sent over the air. Neither shows up at build
# time, so this runs as part of the child build.
#
# GameParty.cpp is preprocessed with SKYJO_CLIENT_ONLY so that host-only asset
# loads, which the guest never reaches, are correctly excluded.

import os
import re
import struct
import subprocess
import sys

BLOCKSDS = '/opt/wonderful/thirdparty/blocksds/core'
BLOCKSDSEXT = '/opt/wonderful/thirdparty/blocksds/external'
GCC = '/opt/wonderful/toolchain/gcc-arm-none-eabi/bin/arm-none-eabi-g++'

CARDS = [f'card_{n}' for n in
         ['n2', 'n1', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
          '10', '11', '12', 'back']]

# GamePartySharedAssets builds these from a CardType at runtime, so they never
# appear as literals anywhere.
RUNTIME_BUILT = ({f'cards/{c}_png.grf' for c in CARDS} |
                 {f'cards2/{c}_png.grf' for c in CARDS})


def referenced():
    """Asset paths the client build can reach, from preprocessed sources."""
    paths = set(RUNTIME_BUILT)
    for src in ('../source/GameParty.cpp', '../source/Music.cpp', 'source/main.cpp'):
        out = subprocess.run(
            [GCC, '-E', '-P', '-DSKYJO_CLIENT_ONLY', '-D__NDS__', '-DARM9',
             '-DNEA_MAXMOD', '-std=gnu++26', '-fno-exceptions', '-fno-rtti',
             '-isystem', f'{BLOCKSDS}/libs/libnds/include',
             '-isystem', f'{BLOCKSDS}/libs/maxmod/include',
             '-isystem', f'{BLOCKSDSEXT}/dswifi_dl/include',
             '-isystem', f'{BLOCKSDSEXT}/nitro-engine-advanced/include',
             '-I../source', '-Isource', '-I../build/assets/arm9',
             '-D_LITTLE_ENDIAN', '-include', 'yas/detail/config/endian.hpp', src],
            capture_output=True, text=True)
        if out.returncode != 0:
            print(f'FAIL  could not preprocess {src}:\n{out.stderr[-800:]}',
                  file=sys.stderr)
            raise SystemExit(2)
        # The character class has to allow '-' and '.': the font is called
        # DejaVuSans-Bold.fnt.
        # .ima is the guest's encoded music track, opened by Music::Play.
        paths |= set(re.findall(r'"([A-Za-z0-9_./-]+\.(?:grf|fnt|npe|ima))"',
                                out.stdout))
        # The soundbank is opened by maxmod, not by a GRF loader.
        paths |= set(re.findall(r'"(maxmod/[A-Za-z0-9_./-]+\.bin)"', out.stdout))
    return paths


def packed(path='generated/assets_blob/assets.bin'):
    with open(path, 'rb') as f:
        blob = f.read()
    magic, count = struct.unpack('<II', blob[:8])
    if magic != 0x5241534B:
        print(f'FAIL  {path} is not a Skyjo asset archive', file=sys.stderr)
        raise SystemExit(2)
    names = set()
    for i in range(count):
        off = struct.unpack('<I', blob[8 + i * 16:12 + i * 16])[0]
        names.add(blob[off:blob.index(b'\0', off)].decode())
    return names


def main():
    want, have = referenced(), packed()
    missing = sorted(want - have)
    extra = sorted(have - want)

    print(f'  client loads {len(want)} assets, archive holds {len(have)}')
    for m in missing:
        print(f'  MISSING  {m}  (the guest would terminate on this)')
    for e in extra:
        print(f'  UNUSED   {e}  (sent over the air for nothing)')

    if missing:
        return 1
    if not extra:
        print('  OK    archive matches the client exactly')
    return 0


if __name__ == '__main__':
    sys.exit(main())
