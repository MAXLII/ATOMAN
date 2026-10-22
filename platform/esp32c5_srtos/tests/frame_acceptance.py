"""Exercise the actual base demo through 2 FRAME sessions and the raw UDP BSP."""
from __future__ import annotations

import argparse
import binascii
import hashlib
import json
import os
from pathlib import Path
import queue
import re
import socket
import struct
import subprocess
import threading
import time

ROOT = Path(__file__).resolve().parents[1]


class FrameShell:
    def __init__(self, executable, out, label, report):
        environment = os.environ.copy()
        environment["FRAME_DATA_DIR"] = str(out / label)
        self.process = subprocess.Popen([str(executable), "shell", "--json"], stdin=subprocess.PIPE,
            stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, encoding="utf-8", env=environment)
        self.lines = queue.Queue()
        self.label, self.report = label, report

        def collect():
            for line in self.process.stdout:
                try:
                    self.lines.put(json.loads(line))
                except ValueError:
                    pass

        threading.Thread(target=collect, daemon=True).start()

    def command(self, text, expected=True):
        self.process.stdin.write(text + "\n")
        self.process.stdin.flush()
        response = self.lines.get(timeout=30)
        self.report["commands"].append({"frame": self.label, "command": text, "result": response})
        assert response.get("ok") is expected, response
        return response.get("data")

    def close(self):
        if self.process.poll() is None:
            self.process.stdin.write("exit\n")
            self.process.stdin.flush()
            try:
                self.process.wait(timeout=10)
            except subprocess.TimeoutExpired:
                self.process.terminate()
                self.process.wait(timeout=5)


def wire(word, payload=b"", destination=2):
    body = bytes([0xe8, 1, 1, 0, destination, 0, 1, word, 0]) + struct.pack("<H", len(payload)) + payload
    return body + struct.pack("<H", binascii.crc_hqx(body, 0xffff)) + b"\r\n"


def received(data):
    return b"".join(bytes.fromhex(row["hex"]) for row in data["records"] if row["dir"] == "rx")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--frame", type=Path, default=Path("D:/OneDrive/LWX/FRAME/build/app/frame.exe"))
    parser.add_argument("--port", default="COM7")
    parser.add_argument("--host")
    parser.add_argument("--output", type=Path, default=ROOT / "build/frame-acceptance")
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    report = {"passed": False, "hardware_tested": True, "port": args.port,
              "baud": 115200, "address": 2, "tcp_port": 5000, "checks": [], "commands": []}
    shells = []

    def save():
        (args.output / "result.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")

    def check(label, condition):
        report["checks"].append({"name": label, "passed": bool(condition)})
        save()
        assert condition, label
        print("PASS " + label, flush=True)

    def read(shell, name):
        return shell.command("param read --name " + name)["value"]

    def write(shell, name, value):
        return shell.command(f"param write --name {name} --value {value}")

    def raw(shell, payload):
        return received(shell.command("serial raw --hex " + payload.hex() + " --duration 0.2"))

    def wifi_status(shell):
        return raw(shell, b"wifi\n")

    def wait_wifi(shell):
        deadline = time.monotonic() + 25
        while time.monotonic() < deadline:
            text = wifi_status(shell)
            match = re.search(rb"ready=1 ip=([0-9.]+).*error=0 control=0", text)
            if match:
                return match.group(1).decode("ascii")
            time.sleep(0.3)
        raise RuntimeError("Wi-Fi did not become ready: " + repr(text))

    def connect_tcp(label, host):
        shell = FrameShell(args.frame, args.output, label, report)
        shells.append(shell)
        shell.command(f"connect --transport tcp --host {host} --tcp-port 5000 --dst 2 --wire e8 "
                      "--timeout 10000 --response-timeout 1500")
        return shell

    try:
        usb = FrameShell(args.frame, args.output, "FRAME-USB", report)
        shells.append(usb)
        usb.command(f"connect --port {args.port} --baud 115200 --dst 2 --wire e8 "
                    "--timeout 10000 --response-timeout 1500")
        host = args.host or wait_wifi(usb)
        report["host"] = host
        tcp = connect_tcp("FRAME-WIFI", host)
        required = {"DEMO_SHELL_COUNTER", "DEMO_SHELL_GAIN", "DEMO_PERF_LOOP", "WIFI_ENABLE"}
        for shell in (usb, tcp):
            data = shell.command("param list")
            rows = data["rows"] if isinstance(data, dict) else data
            check(shell.label + " enumerates real base demo parameters", required <= {row["name"] for row in rows})
        original = int(read(usb, "DEMO_SHELL_COUNTER"))
        for index in range(16):
            value = (0x12e85678 + index * 0x10101) & 0xffffffff
            write(usb, "DEMO_SHELL_COUNTER", value)
            check(f"USB write/TCP read {index}", int(read(tcp, "DEMO_SHELL_COUNTER")) == value)
            value = 0x89abcdef ^ (index * 0x10101)
            write(tcp, "DEMO_SHELL_COUNTER", value)
            check(f"TCP write/USB read {index}", int(read(usb, "DEMO_SHELL_COUNTER")) == value)
        write(usb, "DEMO_SHELL_COUNTER", original)
        gain = float(read(usb, "DEMO_SHELL_GAIN"))
        write(tcp, "DEMO_SHELL_GAIN", 1.75)
        check("float parameter crosses transports", float(read(usb, "DEMO_SHELL_GAIN")) == 1.75)
        write(usb, "DEMO_SHELL_GAIN", gain)
        for shell in (usb, tcp):
            payload = bytes(range(256))
            reply = raw(shell, wire(0x17, payload))
            check(shell.label + " binary loopback preserves all 256 byte values",
                  len(reply) == 271 and reply[11:-4] == payload and reply[8] == 1
                  and struct.unpack("<H", reply[-4:-2])[0] == binascii.crc_hqx(reply[:-4], 0xffff))
            damaged = bytearray(wire(0x17, b"bad crc"))
            damaged[-4] ^= 1
            check(shell.label + " bad CRC rejected", raw(shell, damaged) == b"")
            check(shell.label + " wrong address rejected", raw(shell, wire(0x17, b"wrong", 3)) == b"")
            nested = wire(0x17, b"nested") + b"DEMO_SHELL_PING\n"
            before = int(read(shell, "DEMO_SHELL_COUNTER"))
            check(shell.label + " nested frame not executed", raw(shell, wire(0x7e, nested)) == b"")
            check(shell.label + " binary payload cannot execute Shell", int(read(shell, "DEMO_SHELL_COUNTER")) == before)
            for action in ("info", "summary", "dictionary", "samples"):
                check(shell.label + " Perf " + action, bool(shell.command("perf " + action + " --filter 0")))
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as udp:
            udp.settimeout(2)
            payload = bytes(range(256)) * 2
            udp.sendto(payload, (host, 5000))
            check("UDP binary loopback preserves 512 bytes", udp.recvfrom(1024)[0] == payload)
            udp.sendto(payload * 2, (host, 5000))
            try:
                udp.recvfrom(2048)
                truncated_rejected = False
            except socket.timeout:
                truncated_rejected = True
            check("oversize UDP datagram rejected without truncated echo", truncated_rejected)
            udp.sendto(b"recovery", (host, 5000))
            check("UDP recovers after truncation", udp.recvfrom(1024)[0] == b"recovery")
        tcp.close()
        time.sleep(0.3)
        with socket.create_connection((host, 5000), timeout=3) as interrupted:
            interrupted.sendall(wire(0x17, b"unfinished")[:12])
        time.sleep(0.3)
        tcp = connect_tcp("FRAME-WIFI-reconnect", host)
        check("new TCP session has no old parser state", int(read(tcp, "DEMO_SHELL_COUNTER")) == original)
        write(usb, "WIFI_ENABLE", 0)
        time.sleep(0.4)
        check("USB remains operational while radio stopped", int(read(usb, "DEMO_SHELL_COUNTER")) == original)
        check("radio OFF reported", b"mode=0 ready=0" in wifi_status(usb))
        tcp.close()
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as closed_probe:
            closed_probe.settimeout(1)
            check("TCP listener closed with radio", closed_probe.connect_ex((host, 5000)) != 0)
        write(usb, "WIFI_ENABLE", 1)
        host = wait_wifi(usb)
        report["reconnected_host"] = host
        tcp = connect_tcp("FRAME-WIFI-radio-restart", host)
        check("Wi-Fi rejoins router and FRAME reconnects", int(read(tcp, "DEMO_SHELL_COUNTER")) == original)
        check("Wi-Fi control request readback", int(read(usb, "WIFI_ENABLE")) == 1)
        firmware = ROOT / "build/esp32c5_demo.bin"
        report["firmware_sha256"] = hashlib.sha256(firmware.read_bytes()).hexdigest()
        report["passed"] = True
    finally:
        for shell in reversed(shells):
            shell.close()
        save()


if __name__ == "__main__":
    main()
