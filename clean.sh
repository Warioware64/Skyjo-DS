#!/usr/bin/env bash
# Clean build outputs (delegates to build.py --clean).

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
exec "${SCRIPT_DIR}/make.sh" --clean "$@"
