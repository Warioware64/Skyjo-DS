#!/usr/bin/env python3

# Equivalent of the project Makefile, expressed with architectds_nea_mod.
#
# Usage (after running ./setup-env.sh and `source env/bin/activate`):
#   python build.py          # build
#   python build.py --help   # see all options

from architectds import *


nitrofs = NitroFS()
nitrofs.add_nflib_bg_8bit(['resources/introBGs/'], out_dir="introTitle/")
#nitrofs.add_ptexconv_tex4x4(['testPtxe4'], out_dir="texture/test")
nitrofs.generate_image()

arm9 = Arm9Binary(
    sourcedirs=['source'],
    includedirs=['source'],
    defines=['NEA_MAXMOD'],
    libs=['NEA', 'mm9', 'nds9'],
    libdirs=[
        '${BLOCKSDS}/libs/libnds',
        '${BLOCKSDS}/libs/maxmod',
        '${BLOCKSDSEXT}/nitro-engine-advanced',
    ],
    cxxflags='-Wall -O2 -std=gnu++26',
)
arm9.generate_elf()

nds = NdsRom(
    binaries=[arm9, nitrofs],
    nds_path='skyjo-nds.nds',
    game_title='SKYJO DS',
    game_subtitle='Version',
)
nds.generate_nds()

nds.run_command_line_arguments()
