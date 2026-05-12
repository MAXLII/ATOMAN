#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
#
# Build the GD32G553 firmware from Linux/WSL using the repository makefile.

set -euo pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MAKEFILE_PATH="${SCRIPT_DIR}/makefile"
OUTPUT_NAME="isp_firmware"
JOBS="${JOBS:-$(nproc)}"
MAKE_QUIET="${MAKE_QUIET:-1}"

usage() {
    echo "Usage: $0 [-b|-r|-c]"
    echo "  -b, build    Incremental build (default)"
    echo "  -r, rebuild  Full rebuild: clean and build"
    echo "  -c, clean    Clean build directory only"
}

require_tool() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo -e "${RED}Error: $1 not found.${NC}"
        exit 1
    fi
}

run_make() {
    local make_args=()

    if [ "${MAKE_QUIET}" = "1" ] && [ "${VERBOSE:-0}" != "1" ]; then
        make_args+=("-s")
    fi

    make "${make_args[@]}" -f "${MAKEFILE_PATH}" -j"${JOBS}" OUTPUT_NAME="${OUTPUT_NAME}" "$@"
}

append_fw_info_with_python() {
    local input_bin="$1"
    local output_bin="$2"

    python3 - "$input_bin" "$output_bin" <<'PY'
import re
import struct
import subprocess
import sys
import time
from pathlib import Path

input_file = Path(sys.argv[1])
output_file = Path(sys.argv[2])
header = Path("tools/fw_info/fw_info.h").read_text(encoding="utf-8", errors="ignore")

def read_define(name: str, default: int = 0) -> int:
    match = re.search(rf"^\s*#\s*define\s+{name}\s+([0-9]+)", header, re.MULTILINE)
    return int(match.group(1)) if match else default

def crc32_raw(data: bytes) -> int:
    crc = 0xFFFFFFFF
    for value in data:
        crc ^= value
        for _ in range(8):
            if crc & 1:
                crc = (crc >> 1) ^ 0xEDB88320
            else:
                crc >>= 1
            crc &= 0xFFFFFFFF
    return crc

hard_ver = read_define("HARD_VER")
vendor = read_define("DEVICE_VENDOR")
release_ver = read_define("RELEASE_VER")
debug_ver = read_define("DEBUG_VER")
version = ((hard_ver << 24) | (vendor << 16) | (release_ver << 8) | debug_ver) & 0xFFFFFFFF

module_id = 0x03
fw_type = 1
try:
    commit = subprocess.check_output(
        ["git", "rev-parse", "--short=16", "HEAD"],
        stderr=subprocess.DEVNULL,
        text=True,
    ).strip()
except Exception:
    commit = "NOGIT"

commit_bytes = commit.encode("ascii", errors="ignore")[:15]
commit_bytes = commit_bytes + bytes(16 - len(commit_bytes))
payload = input_file.read_bytes()
body = struct.pack(
    "<IBII16sB",
    int(time.time()),
    fw_type,
    version,
    len(payload),
    commit_bytes,
    module_id,
)
crc = crc32_raw(body)
footer = body + struct.pack("<I", crc)
output_file.write_bytes(payload + footer)
PY
}

BUILD_MODE="build"
case "${1:-}" in
    "" | "-b" | "build")
        BUILD_MODE="build"
        ;;
    "-r" | "rebuild")
        BUILD_MODE="rebuild"
        ;;
    "-c" | "clean")
        BUILD_MODE="clean"
        ;;
    "-h" | "--help")
        usage
        exit 0
        ;;
    *)
        echo -e "${RED}Error: unsupported argument '${1}'.${NC}"
        usage
        exit 1
        ;;
esac

if [ ! -f "${MAKEFILE_PATH}" ]; then
    echo -e "${RED}Error: makefile not found at ${MAKEFILE_PATH}${NC}"
    exit 1
fi

require_tool make
require_tool arm-none-eabi-gcc
require_tool arm-none-eabi-objcopy
require_tool arm-none-eabi-size

HOST_CC="${HOST_CC:-}"
if [ -z "${HOST_CC}" ]; then
    for candidate in gcc cc clang; do
        if command -v "${candidate}" >/dev/null 2>&1; then
            HOST_CC="${candidate}"
            break
        fi
    done
fi

cd "${SCRIPT_DIR}"

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}      Firmware Build Tool (WSL/GCC)${NC}"
echo -e "${GREEN}========================================${NC}"
echo
echo "Parallel jobs: ${JOBS}"
if [ "${MAKE_QUIET}" = "1" ] && [ "${VERBOSE:-0}" != "1" ]; then
    echo "Make output: quiet"
else
    echo "Make output: verbose"
fi
echo
case "${BUILD_MODE}" in
    build)
        echo "Build mode: build (incremental)"
        ;;
    rebuild)
        echo "Build mode: rebuild (clean + build)"
        ;;
    clean)
        echo "Build mode: clean"
        ;;
esac
echo

if [ "${BUILD_MODE}" = "clean" ]; then
    echo -e "${YELLOW}Step 1: clean build directory...${NC}"
    run_make clean
    echo
    echo -e "${GREEN}Clean completed successfully.${NC}"
    exit 0
fi

if [ "${BUILD_MODE}" = "rebuild" ]; then
    echo -e "${YELLOW}Step 1: clean build directory...${NC}"
    run_make clean
else
    echo -e "${YELLOW}Step 1: skip clean build directory...${NC}"
fi

echo -e "${YELLOW}Step 2: build fw_info tool...${NC}"
if [ -n "${HOST_CC}" ]; then
    rm -f tools/fw_info/fw_info
    "${HOST_CC}" -o tools/fw_info/fw_info tools/fw_info/fw_info.c -Itools/fw_info -DIS_PFC
    FW_INFO_TOOL="./tools/fw_info/fw_info"
    FW_INFO_MODE="tool"
    echo "fw_info built by ${HOST_CC}."
elif command -v python3 >/dev/null 2>&1; then
    FW_INFO_TOOL="python3"
    FW_INFO_MODE="python"
    echo "host C compiler not found; use python3 footer appender."
elif [ -f "./tools/fw_info/fw_info.exe" ]; then
    FW_INFO_TOOL="./tools/fw_info/fw_info.exe"
    FW_INFO_MODE="tool"
    chmod +x "${FW_INFO_TOOL}" 2>/dev/null || true
    echo "host C compiler not found; reuse existing tools/fw_info/fw_info.exe."
else
    echo -e "${RED}Error: host C compiler and python3 not found, and tools/fw_info/fw_info.exe is unavailable.${NC}"
    echo -e "${YELLOW}Install gcc/clang/python3 in WSL or run with HOST_CC=/path/to/compiler.${NC}"
    exit 1
fi

echo -e "${YELLOW}Step 3: build firmware with repository makefile...${NC}"
run_make
echo "Firmware build completed."

echo -e "${YELLOW}Step 4: append firmware info...${NC}"
datetime="$(date +%Y%m%d%H%M%S)"
target_dir="builds/${datetime}"
input_bin="build/demo.bin"
output_bin="${target_dir}/demo.bin"
mkdir -p "${target_dir}"
if [ "${FW_INFO_MODE}" = "python" ]; then
    if ! append_fw_info_with_python "${input_bin}" "${output_bin}"; then
        echo -e "${RED}Error: failed to append firmware info.${NC}"
        exit 1
    fi
else
    fw_info_log="$(mktemp)"
    if ! "${FW_INFO_TOOL}" "${input_bin}" "${output_bin}" >"${fw_info_log}" 2>&1; then
        cat "${fw_info_log}"
        rm -f "${fw_info_log}"
        echo -e "${RED}Error: failed to append firmware info.${NC}"
        exit 1
    fi
    rm -f "${fw_info_log}"
fi

input_size="$(stat -c '%s' "${input_bin}")"
output_size="$(stat -c '%s' "${output_bin}")"
footer_size=$((output_size - input_size))

echo "Firmware footer appended successfully."
echo "  Input file:  ${input_bin}"
echo "  Output file: ${output_bin}"
echo "  Input size:  ${input_size} bytes"
echo "  Output size: ${output_size} bytes"
echo "  Footer size: ${footer_size} bytes"
echo "Output: ${output_bin}"

echo
echo -e "${GREEN}Build finished successfully.${NC}"
