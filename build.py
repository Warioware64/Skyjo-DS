#!/usr/bin/env python3

# Equivalent of the project Makefile, expressed with architectds_nea_mod.
#
# Usage (after running ./setup-env.sh and `source env/bin/activate`):
#   python build.py          # build
#   python build.py --help   # see all options

from architectds import *

import os
import sys

CHILD_ROM = 'child/out/skyjo-child.nds'

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
nitrofs.add_ptexconv(['resources/cards/pngObj/'],      out_dir='cards2/')
nitrofs.add_ptexconv(['resources/inGame/'],      out_dir='ingame/')

#nitrofs.add_grit(['resources/cards/card2/'],  out_dir='cards/')
nitrofs.add_bmfont_fnt(['resources/MainMenu/font/'],    out_dir='mainmenu/font/')
nitrofs.add_files_unchanged(['resources/MainMenu/hex/'], out_dir='mainmenu/hex/')
# Streaming main-menu music (raw WAV, read at runtime via mmStream). Copies the
# whole folder, so the tiny _LICENSE.txt / _README.txt attribution files ride
# along into the image too (harmless).
nitrofs.add_files_unchanged(['resources/music/'], out_dir='music/')
# One-shot in-game sound effects. mmutil builds a maxmod soundbank from the wavs
# into nitro:/maxmod/soundbank.bin and emits soundbank_info.h (SFX_* / MSL_*
# defines). The returned header path is registered as an arm9 build dependency
# below so it exists before the sources that include it are compiled.
soundbank_header = nitrofs.add_mmutil(['resources/audioSound/'])
# The DS Download Play child binary, built by child/build.py as an ordinary ROM
# of its own. It ships inside this ROM so the host can read it at runtime and
# send it over the air (see MultiplayerDlPlayMenu).
#
# The two builds depend on each other in opposite directions: the child links
# the assets this build converts into build/nitrofs, and this build embeds the
# finished child. make.sh therefore runs this script, then the child, then this
# script again. On the first pass the child doesn't exist yet, so it is simply
# left out rather than failing the build -- the second pass picks it up.
if os.path.exists(CHILD_ROM):
    nitrofs.add_files_unchanged(['child/out/'], out_dir='dlplay/')
    # add_files_unchanged copies the file but doesn't register it in the assets
    # barrier the ROM depends on, so ndstool would not re-pack when only the
    # child changed. Register it by hand.
    nitrofs.target_files.append('build/nitrofs/dlplay/skyjo-child.nds')
else:
    print('[*] child ROM not built yet, leaving nitro:/dlplay empty this pass')
nitrofs.generate_image()

arm9 = Arm9Binary(
    sourcedirs=['source'],
    includedirs=['source'],
    defines=defines_,
    # DS Download Play lives only in the dswifi fork, installed as "dswifi_dl"
    # next to stock dswifi. Its headers keep the upstream names (dswifi9.h), so
    # ${BLOCKSDS}/libs/dswifi must NOT be in libdirs as well -- which set of
    # headers won the include order would decide the build.
    libs=['NEA', 'mm9', 'nds9', 'dswifi_dl9d_noip'],
    libdirs=[
        '${BLOCKSDS}/libs/libnds',
        '${BLOCKSDS}/libs/maxmod',
        '${BLOCKSDSEXT}/dswifi_dl',
        '${BLOCKSDSEXT}/nitro-engine-advanced',
    ],
    # -D_LITTLE_ENDIAN + force-including yas's endian config first locks yas to
    # little-endian in every TU *before* any <nds.h> can define _BIG_ENDIAN
    # (which yas would otherwise misread as "big-endian"). Order-independent, so
    # it survives files that include <nds.h> before the project headers.
    cxxflags='-Wall -O2 -std=gnu++26 -D_LITTLE_ENDIAN -include yas/detail/config/endian.hpp',
)
# Make the generated soundbank_info.h available before compiling sources.
arm9.add_header_dependencies([soundbank_header])
arm9.generate_elf()

# The ARM7 core has to come from the same build as the ARM9 library: the two
# halves share the structures DSWifi uses to talk between the CPUs, and
# Wifi_Init() refuses a mismatched pair.
arm7 = Arm7BinaryDefault(
    elf_path='${BLOCKSDSEXT}/dswifi_dl/sys/arm7/arm7_dswifi_dl_maxmod.elf'
)

nds = NdsRom(
    binaries=[arm9, arm7, nitrofs],
    nds_path='skyjo-nds.nds',
    game_title='SKYJO DS',
    game_icon="iconSKYJO.png",
    game_subtitle='Made by Warioware64',
    game_author='github.com/Warioware64',
)
nds.generate_nds()

nds.run_command_line_arguments()
