"""Physical GD32 TX-DMA backpressure: network task progress and queued TCP recovery.

Run exclusively with acquisition stopped. J-Link accesses live memory without
halting the CPU. The original DMA control register is restored on every exit.
"""
import argparse
import ctypes as c
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import socket
import subprocess
import threading
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--frame-root', type=Path, required=True)
    parser.add_argument('--host', default='169.254.2.7')
    parser.add_argument('--elf', type=Path, default=Path(__file__).resolve().parents[1] / 'build_realtime/gd32e507_demo.elf')
    args = parser.parse_args()
    output = args.elf.parent
    symbols = {parts[2]: int(parts[0], 16) for line in subprocess.check_output(
        ['arm-none-eabi-nm', '-n', str(args.elf)], text=True).splitlines()
        if len(parts := line.split()) == 3}
    poll = symbols['g_enet_task_poll_count']
    busy = symbols['g_enet_tcp_tx_busy_count']
    dma_control, dma_poll = 0x40029018, 0x40029004
    command = [r'C:\Program Files\SEGGER\JLink_V936\JLink.exe', '-SelectEmuBySN', '69409716',
               '-Device', 'GD32E507ZET6', '-If', 'SWD', '-Speed', '1000', '-AutoConnect', '1', '-ExitOnError', '1']

    def run_jlink(name, text):
        path = output / (name + '.jlink')
        path.write_text(text, encoding='ascii')
        result = subprocess.run(command + ['-CommandFile', str(path)], capture_output=True, text=True, timeout=15)
        (output / (name + '.log')).write_text(result.stdout + result.stderr, encoding='utf-8')
        if result.returncode:
            raise RuntimeError(result.stdout + result.stderr)
        return result.stdout

    def values(log, address):
        return [int(value, 16) for value in re.findall(rf'{address:08X}\s*=\s*([0-9A-Fa-f]+)', log, re.I)]

    spec = importlib.util.spec_from_file_location('capture', args.frame_root / 'tests/hardware/realtime_scope_capture.py')
    capture = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(capture)
    library = args.frame_root / 'build/native/Release/frame_backend.dll'
    os.add_dll_directory(str(library.parent))
    dll = c.CDLL(str(library))
    dll.frame_create.restype = c.c_void_p
    dll.frame_destroy.argtypes = [c.c_void_p]
    dll.frame_submit.argtypes = [c.c_void_p, c.c_char_p, c.c_uint32, c.POINTER(c.c_uint64)]
    dll.frame_result.argtypes = [c.c_void_p, c.c_uint64, c.c_void_p, c.c_uint32, c.POINTER(c.c_uint32)]
    dll.frame_release.argtypes = [c.c_void_p, c.c_uint64]
    client = capture.Backend(dll)
    original = None
    process = None
    report = dict(passed=False, hardware=True, dma_stall_ms=750)
    try:
        client.command('connect', transport='tcp', host=args.host, tcp_port=5000, dst=2, wire='e8')
        capability = client.command('realtime-scope-control', 'capabilities')
        report['elf_sha256'] = hashlib.sha256(args.elf.read_bytes()).hexdigest()
        assert capability['elf_sha256'].lower() == report['elf_sha256'], 'Firmware differs from ELF'
        client.command('realtime-scope-control', 'stop')
        original = values(run_jlink('dma-before', f'mem32 0x{dma_control:X},1\nq\n'), dma_control)[0]
        assert original & (1 << 13), 'TX DMA was not running before the test'
        path = output / 'dma-pressure.jlink'
        path.write_text(f'w4 0x{dma_control:X},0x{original & ~(1 << 13):X}\n'
                        f'mem32 0x{poll:X},1\nmem32 0x{busy:X},1\nsleep 750\n'
                        f'mem32 0x{poll:X},1\nmem32 0x{busy:X},1\n'
                        f'w4 0x{dma_control:X},0x{original:X}\nw4 0x{dma_poll:X},0\nq\n', encoding='ascii')
        stopped, lines = threading.Event(), []
        process = subprocess.Popen(command + ['-CommandFile', str(path)], stdout=subprocess.PIPE,
                                   stderr=subprocess.STDOUT, text=True)

        def collect():
            for line in process.stdout:
                lines.append(line)

        def fill_descriptors():
            with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as udp:
                while not stopped.is_set():
                    udp.sendto(b'DMA pressure' * 5, (args.host, 5000))
                    stopped.wait(0.005)

        reader = threading.Thread(target=collect, daemon=True)
        reader.start()
        sender = threading.Thread(target=fill_descriptors, daemon=True)
        sender.start()
        durations = []
        try:
            # Commander buffers redirected output. Send throughout its lifetime
            # rather than treating a delayed stdout line as a live event.
            while process.poll() is None:
                started = time.monotonic()
                capability = client.command('realtime-scope-control', 'capabilities')
                durations.append(time.monotonic() - started)
                assert capability['sample_frequency'] == 10000
                time.sleep(0.005)
        finally:
            stopped.set()
            sender.join(timeout=2)
        report['tcp_reply_seconds'] = max(durations)
        process.wait(timeout=10)
        reader.join(timeout=2)
        log = ''.join(lines)
        (output / 'dma-pressure.log').write_text(log, encoding='utf-8')
        assert process.returncode == 0, log
        polls, pressure = values(log, poll), values(log, busy)
        assert len(polls) == len(pressure) == 2, log
        report['completed_network_passes_during_stall'] = (polls[1] - polls[0]) & 0xFFFFFFFF
        report['tcp_busy_returns_during_stall'] = (pressure[1] - pressure[0]) & 0xFFFFFFFF
        assert report['completed_network_passes_during_stall'] > 100
        assert report['tcp_busy_returns_during_stall'] > 0
        assert client.command('realtime-scope-control', 'capabilities')['sample_frequency'] == 10000
        report['passed'] = True
    finally:
        if process is not None and process.poll() is None:
            process.kill()
            process.wait()
        if original is not None:
            restored = run_jlink('dma-restored', f'w4 0x{dma_control:X},0x{original:X}\nw4 0x{dma_poll:X},0\nmem32 0x{dma_control:X},1\nq\n')
            report['dma_restored'] = values(restored, dma_control) == [original]
            report['passed'] = report['passed'] and report['dma_restored']
        client.close()
        (output / 'hardware-nonblocking.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps(report, indent=2))
    assert report['passed'], 'DMA restoration or nonblocking verification failed'


if __name__ == '__main__':
    main()
