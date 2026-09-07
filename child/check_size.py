#!/usr/bin/env python3

# Checks that the child ROM can actually be sent with DS Download Play.
#
# Mirrors what Wifi_DlPlayRomParse() does in the dswifi fork (source/arm9/ntr/
# dlplay/rom.c): only the ROM header and the two CPU binaries cross the wire, so
# what matters is their total, not the size of the .nds file.
#
# It also weighs what the program occupies once it is RUNNING, which is a
# different number and, being invisible on the air, the easier one to get wrong.
# .bss costs nothing to transmit and every byte in RAM: a 512 KB buffer moved
# into it once made this ROM 483 KB cheaper to send and 40 KB taller in memory
# at the same time, and the taller one is what broke -- a Download Station boots
# the program into RAM it is still using itself, so a .bss that reaches further
# than the last build's is a real risk that nothing here used to notice.
#
# Usage: python check_size.py [out/skyjo-child.nds]

import struct
import sys

NDS_HDR_SIZE = 0x160

# rom.c:105-107 -- the client loads the program into main RAM, so it can't be
# bigger than main RAM.
MAX_PROGRAM_SIZE = 0x400000

# protocol.c:19 -- the client receives the ARM7 binary into main RAM here and
# relocates it just before boot, so the ARM9 must not reach this address.
# Nothing in the library checks it.
ARM7_STAGING_DEST = 0x022C0000

# dlplay.h:57-63 -- 504 bytes per block for rooms of 11 clients or fewer.
BLOCK_SIZE = 0x01FE - 6

# dlplay.h:323 -- the host queues DLPLAY_BLOCKS_PER_UPDATE blocks per
# Wifi_DlPlayUpdate(), which the menu calls once a frame. Blocks are broadcast,
# so this is how long a guest waits regardless of how many are downloading.
BLOCKS_PER_SECOND = 3 * 60


def bss_size(elf='build/arm9/arm9.elf'):
    """Bytes of .bss in the linked ARM9, or None if the ELF isn't there.

    Read straight out of the section headers rather than through readelf, so
    this stays a plain script with no toolchain lookup of its own.
    """
    try:
        with open(elf, 'rb') as f:
            data = f.read()
    except OSError:
        return None

    if data[:4] != b'\x7fELF' or data[4] != 1:      # 32-bit ELF only
        return None

    e_shoff, = struct.unpack_from('<I', data, 0x20)
    e_shentsize, e_shnum, e_shstrndx = struct.unpack_from('<HHH', data, 0x2E)
    if e_shoff == 0 or e_shnum == 0:
        return None

    def section(i):
        off = e_shoff + i * e_shentsize
        name, typ, flags, addr, offset, size = struct.unpack_from('<IIIIII', data, off)
        return name, typ, addr, size

    # The section-header string table's own file offset, so section names can be
    # read back out: it is the 5th word of its section header.
    sh_str_off, = struct.unpack_from('<I', data,
                                     e_shoff + e_shstrndx * e_shentsize + 0x10)

    for i in range(e_shnum):
        name_off, typ, addr, size = section(i)
        end = data.index(b'\0', sh_str_off + name_off)
        if data[sh_str_off + name_off:end] == b'.bss':
            return size
    return None


def main() -> int:
    path = sys.argv[1] if len(sys.argv) > 1 else 'out/skyjo-child.nds'
    with open(path, 'rb') as f:
        rom = f.read()

    if len(rom) < NDS_HDR_SIZE:
        print(f'FAIL  {path}: shorter than an NDS header')
        return 1

    arm9_off, arm9_entry, arm9_ram, arm9_size = struct.unpack('<4I', rom[0x20:0x30])
    arm7_off, arm7_entry, arm7_ram, arm7_size = struct.unpack('<4I', rom[0x30:0x40])

    # Each segment starts on its own block and its last block is sent short.
    blocks = sum((size + BLOCK_SIZE - 1) // BLOCK_SIZE
                 for size in (NDS_HDR_SIZE, arm9_size, arm7_size))
    padded = blocks * BLOCK_SIZE
    arm9_end = arm9_ram + arm9_size

    print(f'{path}')
    print(f'  file on disk      {len(rom):>9,} bytes')
    print(f'  header             {NDS_HDR_SIZE:>9,}')
    print(f'  ARM9               {arm9_size:>9,}  at 0x{arm9_ram:08X}-0x{arm9_end:08X}')
    print(f'  ARM7               {arm7_size:>9,}  at 0x{arm7_ram:08X}')
    print(f'  ---')
    print(f'  transmitted        {NDS_HDR_SIZE + arm9_size + arm7_size:>9,} '
          f'({blocks} blocks, {padded:,} padded)')
    print(f'  download time      {blocks / BLOCKS_PER_SECOND:>9.1f}s  '
          f'(at {BLOCKS_PER_SECOND} blocks/s)')
    print(f'  budget             {MAX_PROGRAM_SIZE:>9,}  '
          f'({100.0 * padded / MAX_PROGRAM_SIZE:.1f}% used)')

    # What it occupies once it runs: the loaded image plus the .bss the C
    # runtime zeroes before main(). This is the number a Download Station cares
    # about, and the one that does not show up anywhere above.
    bss = bss_size()
    if bss is not None:
        ram_end = arm9_end + bss
        print(f'  ---')
        print(f'  .bss               {bss:>9,}  (zeroed at boot, never sent)')
        print(f'  RAM footprint      {arm9_end - arm9_ram + bss:>9,}  '
              f'ARM9 ends 0x{ram_end:08X}')

    failed = False
    if padded > MAX_PROGRAM_SIZE:
        print(f'FAIL  over the {MAX_PROGRAM_SIZE:,} byte limit; '
              f'Wifi_DlPlayStart() would return -1')
        failed = True
    if arm9_end > ARM7_STAGING_DEST:
        print(f'FAIL  ARM9 reaches 0x{arm9_end:08X}, past the ARM7 staging '
              f'buffer at 0x{ARM7_STAGING_DEST:08X}')
        failed = True
    if bss is not None and (arm9_end + bss) > ARM7_STAGING_DEST:
        print(f'FAIL  ARM9 + .bss reaches 0x{arm9_end + bss:08X}, past the ARM7 '
              f'staging buffer at 0x{ARM7_STAGING_DEST:08X}. Nothing on the air '
              f'shows this -- .bss is zeroed at boot, not transmitted.')
        failed = True

    if not failed:
        print('OK    fits')
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
