#!/usr/bin/env python3

# Build script for the DS Download Play *child* binary.
#
# This is an ordinary architectds ROM, built the regular way, just from its own
# directory: architectds' output paths (build/arm9, build/assets, ...) are
# relative to the working directory, so running this from child/ keeps its build
# tree entirely separate from the parent game's.
#
# The finished ROM is copied into out/, which the parent's build.py drops into
# nitro:/dlplay/ so the host can read it at runtime and send it over the air.
#
# Usage (from this directory, with the project venv active):
#   python build.py

from architectds import *

import os
import subprocess
import sys

if '--debug' in sys.argv:
    defines_ = ['NEA_MAXMOD', 'DEBUG_BUILD']
    dswifi_lib = 'dswifi_dl9d_noip'
else:
    defines_ = ['NEA_MAXMOD']
    dswifi_lib = 'dswifi_dl9_noip'

# No NitroFS. DS Download Play transmits only the ROM header and the two CPU
# binaries -- never the filesystem, the banner or the overlays -- so anything in
# a filesystem here would be dead weight that the guest could never read.
# Every asset the child draws is linked into the ARM9 binary instead.
#
# The assets come from the parent's build/nitrofs, so this build consumes
# exactly what the parent already produced (compressed by ptexconv) rather than
# converting anything a second time. pack_assets.py bundles them into one blob,
# compressing whatever is worth compressing, and AssetDevice.cpp serves that
# blob as a read-only filesystem so every fopen()-based loader keeps working.

ASSETS_ROOT = '../build/nitrofs'

CARD_NAMES = [f'card_{n}' for n in
              ['n2', 'n1', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
               '10', '11', '12', 'back']]

ASSETS = (
    # Bottom-screen 3D card materials and top-screen hardware sprite assets.
    [f'cards/{n}_png.grf' for n in CARD_NAMES] +
    [f'cards2/{n}_png.grf' for n in CARD_NAMES] +
    [
        'mainmenu/hex_background2_png.grf',          # main engine (bottom screen)
        'mainmenu/hex_background_png.grf',           # sub engine (top screen)
        'ingame/clear_png.grf',                      # "not possible" icon
        'mainmenu/font/DejaVuSans-Bold.fnt',         # rich text metadata
        'mainmenu/font/DejaVuSans-Bold_0_png.grf',   # rich text bitmap
        'maxmod/soundbank.bin',                      # the three in-game effects
    ]
)

# Generated inputs live outside build/, not inside it.
#
# Everything under build/ belongs to ninja: architectds registers each output
# directory as a target, so `ninja -t clean` deletes them. These two files are
# made here, by build.py, at generation time -- there is no ninja rule that can
# recreate them. Keeping them under build/ meant a clean deleted them and then
# nothing could put them back ("assets.bin, needed by assets_bin.h, missing and
# no known rule to make it"), while the ones ninja did not recognise stopped it
# removing build/ at all ("remove(build): Directory not empty").
#
# generated/ is ours: ninja never touches it, and build.py refreshes it on every
# run, which is always before ninja needs it.
ASSETS_BLOB = 'generated/assets_blob/assets.bin'

# The music, which cannot come from the parent's nitrofs the way everything
# above does. There it is 5.3 MB of 11025 Hz stereo PCM streamed off the card;
# here it would have to travel inside this binary at ~90 KB/s, so encode_music.py
# reduces it to 8000 Hz mono IMA-ADPCM -- 485 KB, and no decompression buffer,
# 4-bit IMA-ADPCM at 8 kHz, which is a quarter of the bytes of the mono PCM it
# decodes to and needs no decompression buffer at fopen() time -- which is why it
# is packed with --store rather than left to the codec picker.
# See the reasoning at the top of encode_music.py.
MUSIC_SRC = '../resources/music/gameMusic.wav'
MUSIC_IMA = 'generated/music/gameMusic.ima'
MUSIC_NAME = 'music/gameMusic.ima'

# Must match kMusicCapBytes in source/Net/NetProtocol.hpp: the guest sizes its
# receive buffer with that constant and refuses any track claiming to be larger,
# so a track that outgrew it would be silently dropped by every guest.
MUSIC_CAP_BYTES = 512 * 1024


# Nothing to prepare when we are only being asked to clean, and trying anyway
# fails: the parent is cleaned first, so the converted assets pack_assets.py
# reads from ../build/nitrofs have just been deleted. The rules still come out
# right because generated/ is not cleaned, so whatever the last real build put
# there is still on disk for add_data() to find.
CLEANING = '-c' in sys.argv or '--clean' in sys.argv

if not CLEANING:
    if subprocess.run([sys.executable, 'encode_music.py',
                       MUSIC_SRC, MUSIC_IMA]).returncode != 0:
        raise SystemExit('encoding the child music failed')

    os.makedirs(os.path.dirname(ASSETS_BLOB), exist_ok=True)
    if subprocess.run([sys.executable, 'pack_assets.py',
                       ASSETS_ROOT, ASSETS_BLOB] + ASSETS +
                      [f'{MUSIC_NAME}={MUSIC_IMA}', '--store', MUSIC_NAME]
                      ).returncode != 0:
        raise SystemExit('packing the child assets failed')

# The client half of the game, compiled straight out of the parent's source
# tree with SKYJO_CLIENT_ONLY defined. GameParty.cpp guards its host half behind
# that, so the rules engine, the pause and end-game menus, the CPU strategies
# and the suspend-save never reach this binary.
#
# What is deliberately absent: main.cpp and Process.cpp (the child has its own
# entry point), MainMenu.cpp and every MainMenuClasses screen, Intro.cpp, and
# the CpuController / CpuStrategies / RemoteController files, which nothing
# here references once BuildControllers is guarded out.
# Paths go through the `shared` symlink to ../source rather than using ".."
# directly: architectds turns a source path straight into an object path under
# build/, and a leading ".." there collapses into directory targets that collide
# ("multiple rules generate build").
CLIENT_SOURCES = [
    'shared/GameParty.cpp',
    'shared/GamePartyClasses/GamePartySharedAssets.cpp',
    'shared/GamePartyClasses/HumanTouchController.cpp',
    'shared/Net/NetLink.cpp',
    'shared/AssetLoader.cpp',
    'shared/Music.cpp',
    'shared/ErrorHandler.cpp',
]

arm9 = Arm9Binary(
    sourcedirs=['source'],
    sourcefiles=CLIENT_SOURCES,
    # ../build/assets/arm9 is where the parent's mmutil step writes
    # nitrofs/soundbank_info.h (the SFX_* ids). Using the parent's copy rather
    # than running mmutil again keeps those ids matched to the very soundbank.bin
    # this build packs into the archive.
    includedirs=['source', 'shared', '../build/assets/arm9'],
    defines=defines_ + ['SKYJO_CLIENT_ONLY'],
    # Same fork as the parent, and deliberately without ${BLOCKSDS}/libs/dswifi:
    # the fork keeps the upstream header names, so having both in libdirs would
    # leave the include order to decide which library's headers are used.
    libs=['NEA', 'mm9', 'nds9', dswifi_lib],
    libdirs=[
        '${BLOCKSDS}/libs/libnds',
        '${BLOCKSDS}/libs/maxmod',
        '${BLOCKSDSEXT}/dswifi_dl',
        '${BLOCKSDSEXT}/nitro-engine-advanced',
    ],
    # Same yas endian pinning as the parent: NetLink serializes with yas, and
    # without this it misreads the DS as big-endian. See build.py.
    cxxflags='-Wall -O2 -std=gnu++26 -D_LITTLE_ENDIAN '
             '-include yas/detail/config/endian.hpp',
)
# Links the blob as assets_bin[] / assets_bin_size, which AssetDevice.cpp reads.
arm9.add_data([os.path.dirname(ASSETS_BLOB)])
arm9.generate_elf()

# Must be the core from the same build as the ARM9 library, or Wifi_Init()
# refuses the pair.
arm7 = Arm7BinaryDefault(
    elf_path='${BLOCKSDSEXT}/dswifi_dl/sys/arm7/arm7_dswifi_dl_maxmod.elf'
)

# The icon is not just for booting this from a card. It never crosses the air --
# only the header and the two binaries do -- but the host reads the banner out of
# its own in-memory copy of this file (Wifi_DlPlayBeaconSetInfo, beacon_info.c:54:
# header 0x68, then the 0x200-byte icon, its 0x20-byte palette and the English
# title) and re-broadcasts them in the beacon fragments the guest's Download Play
# menu renders. NetLink passes icon_bitmap = NULL precisely so that happens.
# Without this the guests are offered the generic BlocksDS placeholder.
#
# It costs nothing: the banner sits after both binaries in the ROM, so the
# transmitted size is unchanged.
# Built as a Nintendo DS program and nothing else (ndstool -uc 0), rather than
# the DS+DSi hybrid that is the default.
#
# A guest is the one place where a hybrid header buys nothing and costs
# something. Download Play transmits only the header and the two CPU binaries,
# so the DSi halves of a hybrid -- the ARM9i and ARM7i binaries -- never reach
# the console that runs this. Declaring support for a console and then not
# sending it the code is a claim this ROM cannot honour, and it leaves start-up
# taking decisions about hardware that is not there.
#
# Worth knowing about what this does and does not settle: it makes the ROM
# honest and it keeps a DSi in DS mode, but libnds decides DS-versus-DSi at
# start-up by reading SCFG at 0x04004000 -- an address that does not exist on a
# DS -- not by reading this field. The header does not change that read.
nds = NdsRom(
    binaries=[arm9, arm7],
    nds_path='out/skyjo-child.nds',
    game_title='SKYJO DS',
    game_icon='../iconSKYJO.png',
    game_subtitle='SKYJO card board game',
    game_author='github.com/Warioware64',
    unit_code=0,
)
nds.generate_nds()

nds.run_command_line_arguments()
