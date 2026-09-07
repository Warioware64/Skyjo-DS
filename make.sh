#!/usr/bin/env bash
# Build the NDS ROM by running build.py inside the project's Python venv.
# Forwards any extra arguments to build.py (e.g. ./make.sh --ninja, --graph).

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VENV_DIR="${SCRIPT_DIR}/env"

if [ ! -f "${VENV_DIR}/bin/activate" ]; then
    echo "ERROR: Python venv not found at ${VENV_DIR}." >&2
    echo "Run ./setup-env.sh first." >&2
    exit 1
fi

# shellcheck disable=SC1091
source "${VENV_DIR}/bin/activate"

CLEANING=0
for arg in "$@"; do
    case "${arg}" in
        -c|--clean) CLEANING=1 ;;
    esac
done

# ninja is told about object files but not about the libraries they link
# against, so rebuilding libNEA or the dswifi fork leaves the previous ROM in
# place unless some game source happened to change too -- which quietly hands
# you a binary that does not contain the library fix you just made. Drop the
# ELF when a library is newer than it, so the link is redone.
BLOCKSDS="${BLOCKSDS:-/opt/blocksds/core}"
BLOCKSDSEXT="${BLOCKSDSEXT:-/opt/blocksds/external}"

# Compares what the libraries are *now* against what they were at the last link,
# rather than asking whether any of them is newer than the ELF. "Newer" only
# catches an upgrade: putting an older library back -- reverting a fork to the
# stock build, switching branches -- leaves the ELF newer than everything and
# silently keeps the code that was just reverted, which is the worst version of
# this problem because the source looks right.
relink_if_libs_changed() {
    local elf="$1"
    shift

    local stamp="${elf}.libs"
    local current lib
    current="$(for lib in "$@"; do
        if [ -e "${lib}" ]; then
            stat -c '%n %s %Y' "${lib}"
        else
            echo "${lib} absent"
        fi
    done)"

    if [ -f "${elf}" ] && { [ ! -f "${stamp}" ] || [ "${current}" != "$(cat "${stamp}")" ]; }; then
        echo "[*] linked libraries changed since $(basename "${elf}") was built; forcing a relink"
        rm -f "${elf}"
    fi

    mkdir -p "$(dirname "${stamp}")" 2>/dev/null || true
    printf '%s\n' "${current}" > "${stamp}" 2>/dev/null || true
}

LINKED_LIBS=(
    "${BLOCKSDS}/libs/libnds/lib/libnds9.a"
    "${BLOCKSDS}/libs/libnds/lib/libnds9d.a"
    "${BLOCKSDSEXT}/dswifi_dl/lib/libdswifi_dl9_noip.a"
    "${BLOCKSDSEXT}/dswifi_dl/lib/libdswifi_dl9d_noip.a"
    "${BLOCKSDSEXT}/dswifi_dl/sys/arm7/arm7_dswifi_dl_maxmod.elf"
    "${BLOCKSDSEXT}/nitro-engine-advanced/lib/libNEA.a"
    "${BLOCKSDSEXT}/nitro-engine-advanced/lib/libNEA_debug.a"
)

relink_if_libs_changed "${SCRIPT_DIR}/build/arm9/arm9.elf" "${LINKED_LIBS[@]}"
relink_if_libs_changed "${SCRIPT_DIR}/child/build/arm9/arm9.elf" "${LINKED_LIBS[@]}"

# architectds runs ninja with no job limit, so it uses CPUs+2. These are heavy
# translation units -- gnu++26 plus ~930 KB of yas headers apiece -- and enough
# of them at once gets the compiler killed by the kernel mid-build. Cap the jobs
# by memory that is actually free, allowing roughly 1.2 GB per compiler.
# Override with SKYJO_JOBS=n.
if [ -z "${SKYJO_JOBS:-}" ]; then
    avail_mb="$(awk '/^MemAvailable:/ { print int($2 / 1024) }' /proc/meminfo)"
    by_cpu="$(nproc)"
    SKYJO_JOBS=$(( avail_mb / 1200 ))
    [ "${SKYJO_JOBS}" -gt "${by_cpu}" ] && SKYJO_JOBS="${by_cpu}"
    [ "${SKYJO_JOBS}" -lt 1 ] && SKYJO_JOBS=1
fi
if [ "${CLEANING}" -eq 0 ]; then
    echo "[*] building with ${SKYJO_JOBS} parallel job(s)"
fi

# ninja is run directly rather than through architectds, which does not expose
# a job limit. These are the variables it would have passed.
export BLOCKSDS="${BLOCKSDS}"
export BLOCKSDSEXT="${BLOCKSDSEXT}"
export WONDERFUL_TOOLCHAIN="${WONDERFUL_TOOLCHAIN:-/opt/wonderful}"

BUILD_ARGS=("$@")

# A clean must not go on to build. build.py performs the clean itself (it runs
# `ninja -t clean`), and running ninja straight afterwards asks it to build a
# tree that was just emptied -- which fails, because some inputs are produced by
# build.py at generation time rather than by any ninja rule. clean.sh forwards
# --clean here, and the editor's Rebuild task is Clean followed by Build, so the
# build it needs comes from that second step.
generate_and_build() {
    cd "$1"
    python build.py -n "${BUILD_ARGS[@]}" > /dev/null
    if [ "${CLEANING}" -eq 0 ]; then
        ninja -f build.ninja -j "${SKYJO_JOBS}"
    fi
}

# Assets, child binary, then the final ROM.
#
# The two builds point at each other: child/build.py links the converted assets
# this build produces in build/nitrofs, and this ROM embeds the finished child
# at nitro:/dlplay. So the parent runs once to convert assets, the child is
# built against them, and the parent runs again to pack it all up. The second
# parent pass is nearly free -- ninja only re-runs ndstool.
generate_and_build "${SCRIPT_DIR}"

generate_and_build "${SCRIPT_DIR}/child"

if [ "${CLEANING}" -eq 1 ]; then
    echo "[*] cleaned"
    exit 0
fi

python check_assets.py
python check_size.py

generate_and_build "${SCRIPT_DIR}"
