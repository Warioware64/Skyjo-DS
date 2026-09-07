#!/usr/bin/env python3

# Packs the child binary's assets into one blob that gets linked into its ARM9.
#
# A Download Play guest has no filesystem, so the game's assets travel inside the
# binary. AssetDevice.cpp serves this blob through a BlocksDS device_io_t, which
# means every existing fopen()/NEA_*LoadGRFFAT() path keeps working unchanged.
#
# Entries are compressed with the Nintendo-format packers in /opt/wonderful/bin
# (the same BIOS formats libnds' decompress() understands), and only when that
# actually comes out smaller. The .grf files are already compressed chunk by
# chunk by ptexconv, so they get stored as-is; this mainly picks up the maxmod
# soundbank, which isn't a GRF and has no compression of its own.
#
# Usage: python pack_assets.py <root_dir> <out_file> <entry> [<entry> ...]
#                             [--store <name>]...
#   root_dir  directory the paths are relative to (the parent's build/nitrofs)
#   out_file  blob to write
#   entry     "path", a file under root_dir named in the archive by that path,
#             or "name=path" for a file elsewhere (a generated one, say) that
#             should appear in the archive under `name`
#   --store   name of an entry to store verbatim, never compressed; repeatable

import os
import struct
import subprocess
import sys
import tempfile

MAGIC = 0x5241534B  # 'KSAR' little-endian: "SKyjo ARchive"

# Nintendo-format compressors shipped with the wonderful toolchain. Each writes
# the 4-byte BIOS header (type in bits 4-7, decompressed size in bits 8-31) that
# libnds' decompress() expects, so the runtime dispatches on the same value that
# grf.c does for GRF chunks.
#
# -ewo is LZSS "WRAM, optimal": the archive always decompresses into a malloc'd
# buffer, never straight into VRAM, so the VRAM-safe variant would only cost
# bytes for nothing. Huffman often wins on sample data; RLE is tried because it
# occasionally wins on very repetitive files and costs nothing to check.
CODECS = [
    ('lzss',    ['/opt/wonderful/bin/wf-nnpack-lzss', '-ewo']),
    ('huffman', ['/opt/wonderful/bin/wf-nnpack-huffman', '-e0']),
    ('rle',     ['/opt/wonderful/bin/wf-nnpack-rle', '-e']),
]


# A compressed entry is decompressed into a malloc'd buffer when it is opened,
# so a marginal saving buys a few bytes over the air and costs an allocation and
# a decompression pass every time. The .grf files are already compressed chunk
# by chunk, so squeezing them again typically saves a few dozen bytes -- not
# worth it. Only take a win that is worth having.
MIN_SAVING_BYTES = 64
MIN_SAVING_RATIO = 0.05


def compress(data):
    """Returns the smallest BIOS-compressed form of `data`, or None if none of
    the codecs beats storing it raw by a worthwhile margin."""
    best = None
    with tempfile.TemporaryDirectory() as tmp:
        src = os.path.join(tmp, 'in.bin')
        with open(src, 'wb') as f:
            f.write(data)

        for name, cmd in CODECS:
            dst = os.path.join(tmp, f'out.{name}')
            try:
                subprocess.run(cmd + [src, dst], check=True,
                               stdout=subprocess.DEVNULL,
                               stderr=subprocess.DEVNULL)
            except (subprocess.CalledProcessError, FileNotFoundError):
                continue
            if not os.path.exists(dst):
                continue
            with open(dst, 'rb') as f:
                out = f.read()
            if len(out) >= len(data):
                continue

            # Trust nothing: the header has to declare exactly the size we
            # started with, or the runtime would allocate the wrong buffer.
            header = struct.unpack('<I', out[:4])[0]
            if (header >> 8) != len(data):
                print(f'  warning: {name} declared {header >> 8} bytes for a '
                      f'{len(data)} byte input, skipping', file=sys.stderr)
                continue

            if best is None or len(out) < len(best):
                best = out

    if best is None:
        return None

    saved = len(data) - len(best)
    if saved < MIN_SAVING_BYTES or saved < len(data) * MIN_SAVING_RATIO:
        return None
    return best


def parse_args(argv):
    """Splits the command line into (root, out_path, [(name, path)], {stored}).

    An entry is either "path" (relative to root) or "name=path" for a file that
    lives somewhere else -- the encoded music track is generated into the child's
    own build tree rather than the parent's nitrofs."""
    positional = []
    store = set()

    i = 0
    while i < len(argv):
        arg = argv[i]
        if arg == '--store':
            i += 1
            if i >= len(argv):
                raise SystemExit('pack_assets.py: --store needs a name')
            store.add(argv[i])
        else:
            positional.append(arg)
        i += 1

    if len(positional) < 3:
        raise SystemExit(
            'usage: pack_assets.py <root> <out> <entry>... [--store <name>]...')

    root, out_path = positional[0], positional[1]

    entries = []
    for entry in positional[2:]:
        if '=' in entry:
            name, path = entry.split('=', 1)
        else:
            name, path = entry, os.path.join(root, entry)
        entries.append((name.replace(os.sep, '/'), path))

    unknown = store - {name for name, _ in entries}
    if unknown:
        raise SystemExit('pack_assets.py: --store names nothing packed: '
                         + ', '.join(sorted(unknown)))

    return root, out_path, entries, store


def main():
    _, out_path, sources, store = parse_args(sys.argv[1:])

    entries = []
    total_raw = 0
    total_stored = 0

    for name, path in sources:
        with open(path, 'rb') as f:
            data = f.read()

        # Some entries must not be compressed at all. AssetOpen() hands out a
        # pointer straight into the linked archive for a stored entry but
        # mallocs and decompresses the whole file for a compressed one, so
        # anything the game streams rather than loads in one go has to stay
        # raw -- the music track would otherwise cost half a megabyte of heap
        # the moment fopen() ran, to save 16% of its size over the air.
        packed = None if name in store else compress(data)
        stored = packed if packed is not None else data

        total_raw += len(data)
        total_stored += len(stored)
        entries.append((name, stored, len(data)))

    # Layout: magic, count, a fixed-size record per entry, then the name blob,
    # then the payloads. Payloads are 4-byte aligned because the BIOS
    # decompression routines read their header as a word.
    header_size = 8 + len(entries) * 16

    names = bytearray()
    name_offsets = []
    for name, _, _ in entries:
        name_offsets.append(header_size + len(names))
        names += name.encode('ascii') + b'\0'
    while len(names) % 4:
        names += b'\0'

    blob = bytearray()
    data_base = header_size + len(names)
    data_offsets = []
    for _, stored, _ in entries:
        while (data_base + len(blob)) % 4:
            blob += b'\0'
        data_offsets.append(data_base + len(blob))
        blob += stored

    out = bytearray()
    out += struct.pack('<II', MAGIC, len(entries))
    for i, (_, stored, real) in enumerate(entries):
        # stored_size != real_size marks a compressed entry, and its payload
        # then starts with the BIOS header that says which codec to use.
        out += struct.pack('<IIII', name_offsets[i], data_offsets[i],
                           len(stored), real)
    out += names
    out += blob

    os.makedirs(os.path.dirname(os.path.abspath(out_path)), exist_ok=True)
    with open(out_path, 'wb') as f:
        f.write(out)

    saved = total_raw - total_stored
    pct = (100.0 * total_stored / total_raw) if total_raw else 100.0
    print(f'  packed {len(entries)} files: {total_raw:,} -> {total_stored:,} '
          f'bytes ({pct:.0f}%, saved {saved:,})')
    print(f'  archive {len(out):,} bytes -> {out_path}')
    return 0


if __name__ == '__main__':
    sys.exit(main())
