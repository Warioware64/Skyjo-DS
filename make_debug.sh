#!/usr/bin/env bash
# Debug build of the NDS ROM (and of the Download Play child binary).
#
# Everything make.sh does, with --debug forwarded to both build scripts.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

exec "${SCRIPT_DIR}/make.sh" --debug "$@"
