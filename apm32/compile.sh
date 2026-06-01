#!/usr/bin/env bash
# SPDX-License-Identifier: MIT

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
JOBS="${JOBS:-$(nproc)}"
MAKE_QUIET="${MAKE_QUIET:-1}"

run_make() {
    local make_args=()

    if [ "${MAKE_QUIET}" = "1" ] && [ "${VERBOSE:-0}" != "1" ]; then
        make_args+=("-s")
    fi

    make "${make_args[@]}" -f "${SCRIPT_DIR}/makefile" -j"${JOBS}" "$@"
}

case "${1:-build}" in
    -b|build)
        run_make
        ;;
    -r|rebuild)
        run_make clean
        run_make
        ;;
    -c|clean)
        run_make clean
        ;;
    *)
        echo "Usage: $0 [-b|build|-r|rebuild|-c|clean]" >&2
        exit 1
        ;;
esac
