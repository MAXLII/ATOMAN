#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
#
# Download GD32G553 firmware from WSL using SEGGER J-Link.
#
# By default this script prefers Windows JLink.exe when it is run inside WSL,
# so USB access is handled by Windows and does not require usbipd attach.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

JLINK_DEVICE="${JLINK_DEVICE:-GD32G553RCT6}"
JLINK_IF="${JLINK_IF:-SWD}"
JLINK_SPEED="${JLINK_SPEED:-4000}"
BIN_ADDR="${BIN_ADDR:-0x08000000}"
DEFAULT_FIRMWARE="${DEFAULT_FIRMWARE:-build/demo.bin}"
LOG_FILE="${LOG_FILE:-JLink_demo.log}"
RUN_AFTER_DOWNLOAD="${RUN_AFTER_DOWNLOAD:-1}"

usage() {
    cat <<EOF
Usage:
  ./download.sh [firmware_file]

Default:
  firmware_file: ${DEFAULT_FIRMWARE}

Supported formats:
  .bin, .hex, .s19

Environment:
  JLINK_EXE=/path/to/JLinkExe    Override J-Link executable
  JLINK_BACKEND=windows          Prefer Windows JLink.exe from WSL
  JLINK_BACKEND=linux            Prefer Linux JLinkExe in WSL
  JLINK_DEVICE=GD32G553RCT6   J-Link device name
  JLINK_IF=SWD                J-Link interface
  JLINK_SPEED=4000            J-Link speed in kHz
  BIN_ADDR=0x08000000         Load address for .bin files
  RUN_AFTER_DOWNLOAD=0        Keep MCU halted after download
  RUN_AFTER_DOWNLOAD=1        Run MCU after download
EOF
}

die() {
    echo "[ERROR] $*" >&2
    exit 1
}

find_jlink() {
    if [ -n "${JLINK_EXE:-}" ]; then
        if [ -x "${JLINK_EXE}" ] || [[ "${JLINK_EXE,,}" == *.exe ]]; then
            echo "${JLINK_EXE}"
            return 0
        fi
        return 1
    fi

    local backend="${JLINK_BACKEND:-auto}"
    if [ "${backend,,}" != "linux" ] && grep -qi microsoft /proc/version 2>/dev/null; then
        local windows_candidates=(
            "/mnt/c/Program Files/SEGGER/JLink/JLink.exe"
            "/mnt/c/Program Files (x86)/SEGGER/JLink/JLink.exe"
            /mnt/c/Program\ Files/SEGGER/JLink*/JLink.exe
            /mnt/c/Program\ Files\ \(x86\)/SEGGER/JLink*/JLink.exe
        )
        local candidate
        for candidate in "${windows_candidates[@]}"; do
            if [ -f "${candidate}" ]; then
                echo "${candidate}"
                return 0
            fi
        done
    fi

    if command -v JLinkExe >/dev/null 2>&1; then
        command -v JLinkExe
        return 0
    fi

    if command -v JLink >/dev/null 2>&1; then
        command -v JLink
        return 0
    fi

    return 1
}

make_load_command() {
    local file_path="$1"
    local ext="${file_path##*.}"

    case "${ext,,}" in
        bin)
            echo "loadfile \"${file_path}\" ${BIN_ADDR}"
            ;;
        hex|s19)
            echo "loadfile \"${file_path}\""
            ;;
        *)
            die "Unsupported file extension: .${ext}. Supported formats: .bin, .hex, .s19"
            ;;
    esac
}

is_windows_jlink() {
    local exe="$1"
    [[ "${exe,,}" == *.exe ]] || [[ "${exe}" == /mnt/* ]]
}

to_jlink_path() {
    local file_path="$1"
    local exe="$2"

    if is_windows_jlink "${exe}"; then
        wslpath -w "${file_path}"
    else
        echo "${file_path}"
    fi
}

case "${1:-}" in
    -h|--help)
        usage
        exit 0
        ;;
esac

cd "${SCRIPT_DIR}"

firmware="${1:-${DEFAULT_FIRMWARE}}"
if [ ! -f "${firmware}" ]; then
    echo "[ERROR] Firmware file does not exist: ${firmware}" >&2
    echo >&2
    usage >&2
    exit 1
fi

jlink_exe="$(find_jlink)" || die "J-Link executable not found. Install SEGGER J-Link on Windows or WSL, or set JLINK_EXE."
firmware_abs="$(realpath "${firmware}")"
firmware_jlink_path="$(to_jlink_path "${firmware_abs}" "${jlink_exe}")"
log_jlink_path="$(to_jlink_path "$(realpath -m "${LOG_FILE}")" "${jlink_exe}")"
load_cmd="$(make_load_command "${firmware_jlink_path}")"

run_after_text="Enabled"
case "${RUN_AFTER_DOWNLOAD,,}" in
    0|false|no|off)
        RUN_AFTER_DOWNLOAD=0
        run_after_text="Disabled"
        ;;
    *)
        RUN_AFTER_DOWNLOAD=1
        ;;
esac

jlink_script="$(mktemp)"
cleanup() {
    rm -f "${jlink_script}"
}
trap cleanup EXIT

{
    echo "device ${JLINK_DEVICE}"
    echo "si ${JLINK_IF}"
    echo "speed ${JLINK_SPEED}"
    echo "connect"
    echo "halt"
    echo "r"
    echo "${load_cmd}"
    echo "r"
    if [ "${RUN_AFTER_DOWNLOAD}" = "1" ]; then
        echo "g"
    fi
    echo "exit"
} >"${jlink_script}"

echo "========================================"
echo "        J-Link Download Tool"
echo "========================================"
echo "Device: ${JLINK_DEVICE}"
echo "Interface: ${JLINK_IF}"
echo "Speed: ${JLINK_SPEED} kHz"
echo "Firmware: ${firmware_abs}"
if is_windows_jlink "${jlink_exe}"; then
    echo "Backend: Windows JLink.exe"
else
    echo "Backend: Linux JLinkExe"
fi
echo "Run after download: ${run_after_text}"
if [[ "${firmware_abs,,}" == *.bin ]]; then
    echo "BIN address: ${BIN_ADDR}"
fi
echo "J-Link: ${jlink_exe}"
echo "Log: ${LOG_FILE}"
echo

rm -f "${LOG_FILE}"
"${jlink_exe}" -CommanderScript "${jlink_script}" -Log "${log_jlink_path}"

if [ -f "${LOG_FILE}" ] && grep -Eiq "FAILED:|Cannot connect to the probe|Download failed|Flash download failed|Error while" "${LOG_FILE}"; then
    echo >&2
    echo "[ERROR] J-Link reported a failure. Check log: ${LOG_FILE}" >&2
    exit 1
fi

echo
echo "Download completed successfully."
if [ "${RUN_AFTER_DOWNLOAD}" = "1" ]; then
    echo "MCU is now running."
else
    echo "MCU remains halted."
fi
