#!/usr/bin/env python3

# Equivalent of the project Makefile, expressed with architectds_nea_mod.
#
# Usage (after running ./setup-env.sh and `source env/bin/activate`):
#   python build.py          # build
#   python build.py --help   # see all options

from architectds import *

import sys

argv = sys.argv

if '--debug' in argv:
    defines_=['NEA_MAXMOD', 'DEBUG_BUILD']
else:
    defines_=['NEA_MAXMOD']

nitrofs = NitroFS()
# Hardware 2D backgrounds: grit produces raw .img / .map / .pal files that
# NEA_Hw2DBGLoad*FAT loads directly. 8bpp tiled mode gives the best VRAM
# usage for repeating patterns like the hex backgrounds.
nitrofs.add_grit(['resources/introBGs/'],     out_dir='intro/')
nitrofs.add_grit(['resources/MainMenu/bg/'],  out_dir='mainmenu/')
# Rich-text font for the menu stays as a tex4x4 atlas (3D quad text path).

nitrofs.add_ptexconv(['resources/MainMenu/font/'],      out_dir='mainmenu/font/')
nitrofs.add_ptexconv(['resources/MainMenu/hexMat/'],      out_dir='mainmenu/hex/')
nitrofs.add_ptexconv(['resources/MainMenu/btns/'],      out_dir='mainmenu/btns/')
nitrofs.add_ptexconv(['resources/cards/png/'],      out_dir='cards/')
nitrofs.add_bmfont_fnt(['resources/MainMenu/font/'],    out_dir='mainmenu/font/')
nitrofs.add_files_unchanged(['resources/MainMenu/hex/'], out_dir='mainmenu/hex/')
nitrofs.generate_image()

arm9 = Arm9Binary(
    sourcedirs=['source'],
    includedirs=['source'],
    defines=defines_,
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
