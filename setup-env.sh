#!/usr/bin/env bash
# Create a local Python virtual environment in ./env and install the
# modified architectds package (architectds_nea_mod) into it in editable mode,
# along with Pillow which is required by the nitro-engine-advanced img2ds tool [dont worry img2ds isnt need ;) ].
#
# Usage:
#   ./setup-env.sh
#   source env/bin/activate
#   python build.py    # or whatever build script uses architectds

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VENV_DIR="${SCRIPT_DIR}/env"
PKG_DIR="${SCRIPT_DIR}/architectds_nea_mod"

if [ ! -d "${PKG_DIR}" ]; then
    echo "ERROR: architectds_nea_mod not found at ${PKG_DIR}" >&2
    exit 1
fi

if [ ! -d "${VENV_DIR}" ]; then
    echo "Creating virtual environment at ${VENV_DIR}"
    python3 -m venv "${VENV_DIR}"
fi

# shellcheck disable=SC1091
source "${VENV_DIR}/bin/activate"

python -m pip install --upgrade pip
python -m pip install -e "${PKG_DIR}"
python -m pip install pillow ninja

echo
echo "Done. Activate the environment with:"
echo "    source env/bin/activate"
