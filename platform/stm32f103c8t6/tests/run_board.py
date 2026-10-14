"""Physical USART1 acceptance for either shared SECTION runtime.

Requires pyserial. Builds are separate; --image explicitly enables J-Link flashing.
Results and readback evidence stay inside this platform's ignored build directory.
"""

import argparse
import binascii
import hashlib
import json
import random
import re
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "build_tools"))
import serial


def flash(image, jlink, output):
    image = image.resolve()
    script = output / "acceptance.jlink"
    readback = output / "readback.bin"
    script.write_text(
        f'r\nh\nloadbin "{image.as_posix()}", 0x08000000\n'
        f'verifybin "{image.as_posix()}", 0x08000000\n'
        f'savebin "{readback.as_posix()}", 0x08000000, 0x{image.stat().st_size:x}\n'
        "r\ng\nq\n", encoding="utf-8"
    )
    command = [jlink, "-NoGui", "1", "-device", "STM32F103C8", "-if", "SWD",
               "-speed", "1000", "-autoconnect", "1", "-ExitOnError", "1",
               "-CommandFile", str(script)]
    result = subprocess.run(command, capture_output=True, text=True, timeout=30)
    (output / "flash.log").write_text(result.stdout + result.stderr, encoding="utf-8")
    if result.returncode != 0 or "Verify successful" not in result.stdout:
        raise RuntimeError(f"J-Link flash/verify failed: {output / 'flash.log'}")
    if image.read_bytes() != readback.read_bytes():
        raise AssertionError("Flash readback differs from the built binary")
    return hashlib.sha256(readback.read_bytes()).hexdigest()


def read_until(port, count, timeout=1.0):
    result = bytearray()
    deadline = time.monotonic() + timeout
    while len(result) < count and time.monotonic() < deadline:
        result.extend(port.read(count - len(result)))
    return bytes(result)


def shell(port, command):
    # Binary parsers share this Link with the Shell; flush a partial text line first.
    port.write(b"\r\n" + command.encode("ascii") + b"\r\n")
    line = port.read_until(b"\n", 512).decode("ascii", errors="replace").strip()
    if not line:
        raise AssertionError(f"No Shell reply to {command!r}")
    return line


def fields(line):
    return dict(re.findall(r"(\w+)=([\w-]+)", line))


def frame_e8(payload, reply=False):
    data = bytes([0xE8, 1, 2 if reply else 1, 0, 1 if reply else 2, 0,
                  1, 0x17, int(reply)]) + len(payload).to_bytes(2, "little") + payload
    return data + binascii.crc_hqx(data, 0xFFFF).to_bytes(2, "little") + b"\r\n"


def checksum(data):
    value = sum(data) & 255
    return {0: 0x55, 255: 0xAA}.get(value, value)


def frame_e9(payload, seq):
    msg = seq | (2 << 3) | (1 << 10) | (1 << 17) | (0x17 << 21)
    data = b"\xE9" + msg.to_bytes(4, "little") + bytes([len(payload) & 255]) + payload
    return data + bytes([checksum(data)])


def check_e9(port, payload, seq):
    prefix = read_until(port, 6)
    if len(prefix) != 6 or prefix[0] != 0xE9:
        raise AssertionError(f"Invalid E9 response prefix: {prefix.hex()}")
    msg = int.from_bytes(prefix[1:5], "little")
    expected_msg = seq | (1 << 3) | (2 << 10) | (1 << 17) | (0x17 << 21) | (1 << 31)
    if msg != expected_msg:
        raise AssertionError(f"E9 identity/ACK/codec mismatch: {msg:08x}")
    length = prefix[5] or 256
    rest = read_until(port, length + 1)
    if len(rest) != length + 1 or rest[:-1] != payload:
        raise AssertionError("E9 payload mismatch")
    if checksum(prefix + rest[:-1]) != rest[-1]:
        raise AssertionError("E9 checksum mismatch")


def run(args):
    output = ROOT / f"build_{args.runtime}{'_test' if args.stress else ''}"
    output.mkdir(exist_ok=True)
    result = {"runtime": args.runtime, "port": args.port, "baud": 115200,
              "flash_sha256": flash(Path(args.image), args.jlink, output) if args.image else None}
    rng = random.Random(103)
    with serial.Serial(args.port, 115200, timeout=0.3, write_timeout=1) as port:
        time.sleep(0.3)
        port.reset_input_buffer()
        first = fields(shell(port, "board"))
        wall_start = time.monotonic()
        assert first["runtime"] == args.runtime.replace("_", "-"), first
        assert int(first["clock"]) == 72000000, first
        assert int(first["preemptive"]) == int(args.runtime == "srtos_m"), first
        assert shell(port, "time").startswith("time = ")
        e8_count = 0
        e9_count = 0
        seq = 0
        for length in [1, 16, 64, 127, 256] * 10:
            payload = bytes(rng.randrange(256) for _ in range(length))
            request = frame_e8(payload)
            # Split maximum frames across host writes to exercise persistent parser state.
            port.write(request[:7])
            port.write(request[7:])
            expected = frame_e8(payload, reply=True)
            actual = read_until(port, len(expected))
            assert actual == expected, ("E8 response mismatch", length, actual.hex())
            e8_count += 1
            port.write(frame_e9(payload, seq))
            check_e9(port, payload, seq)
            e9_count += 1
            seq = (seq + 1) & 7
        # Corrupted frames must produce no reply; subsequent valid frames must recover.
        bad = bytearray(frame_e8(b"bad-crc"))
        bad[-4] ^= 1
        port.write(bad)
        assert port.read(512) == b"", "E8 CRC corruption was accepted"
        bad = bytearray(frame_e9(b"bad-sum", seq))
        bad[-1] ^= 1
        port.write(bad)
        assert port.read(512) == b"", "E9 checksum corruption was accepted"
        request = frame_e9(b"recover", seq)
        port.write(request)
        check_e9(port, b"recover", seq)
        port.write(request)
        assert port.read(512) == b"", "E9 duplicate sequence was accepted"
        # A rejected duplicate must drain its binary body and allow the next frame immediately.
        recovery = b"\xe8\x01\xe9\n\r\x00" + bytes(range(64))
        port.write(frame_e8(recovery))
        assert read_until(port, len(frame_e8(recovery, True))) == frame_e8(recovery, True)
        # An interrupted frame must recover at the shared 100 ms deadline.
        port.write(frame_e9(b"truncated", (seq + 1) & 7)[:4])
        time.sleep(0.12)
        port.write(frame_e8(recovery))
        assert read_until(port, len(frame_e8(recovery, True))) == frame_e8(recovery, True)
        result["e8_roundtrips"] = e8_count + 2
        result["e9_roundtrips"] = e9_count + 1
        result["malformed_and_duplicate_rejected"] = True
        result["parser_recovery_pass"] = True
        if args.stress:
            deadline = time.monotonic() + args.seconds
            while time.monotonic() < deadline:
                current = fields(shell(port, "test"))
                assert int(current["errors"]) == 0 and int(current["fault"]) == 0, current
                time.sleep(0.2)
            result["context_test"] = fields(shell(port, "test"))
            assert int(result["context_test"]["checks"]) >= 20, result
            if args.runtime == "srtos_m":
                assert int(result["context_test"]["preemptions"]) >= 20, result
            else:
                assert int(result["context_test"]["preemptions"]) == 0, result
        last = fields(shell(port, "board"))
        wall_elapsed = time.monotonic() - wall_start
        tick_delta = (int(last["tick"]) - int(first["tick"])) & 0xFFFFFFFF
        irq_delta = (int(last["irq"]) - int(first["irq"])) & 0xFFFFFFFF
        assert tick_delta > 10000 and int(last["led"]) > int(first["led"]), last
        assert abs(tick_delta - 10 * irq_delta) <= 20, (tick_delta, irq_delta)
        tick_elapsed = tick_delta / 10000
        assert abs(tick_elapsed / wall_elapsed - 1) < 0.02, (tick_elapsed, wall_elapsed)
        perf_delta = (int(last["perf"]) - int(first["perf"])) & 0xFFFFFFFF
        assert abs(perf_delta / (tick_delta * 7200) - 1) < 0.01, perf_delta
        result["tick_wall_error_percent"] = round(100 * (tick_elapsed / wall_elapsed - 1), 3)
        result["perf_cycles_per_tick"] = round(perf_delta / tick_delta, 2)
        for name in ["txdrop", "rxdrop", "rxerr"]:
            assert int(last[name]) == 0, last
        result["board_before"] = first
        result["board_after"] = last
        result["pass"] = True
    (output / "acceptance.json").write_text(json.dumps(result, indent=2), encoding="utf-8")
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", required=True)
    parser.add_argument("--runtime", choices=["baremetal", "srtos_m"], required=True)
    parser.add_argument("--image", help="Explicitly flash and read back this binary before testing")
    parser.add_argument("--jlink", default=r"C:\Program Files\SEGGER\JLink_V936\JLink.exe")
    parser.add_argument("--stress", action="store_true", help="Require the BOARD_TEST=1 workload")
    parser.add_argument("--seconds", type=float, default=20)
    run(parser.parse_args())
