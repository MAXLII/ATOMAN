"""GD32 physical-board control, sparse-channel, reconnect and upload-ownership checks.

Uses FRAME's production native backend and the board's shared demo RAM. No signal
or device emulator is used. Run exclusively after the long acquisition test.
"""
import argparse
import ctypes as c
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import socket
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--frame-root', type=Path, required=True)
    parser.add_argument('--host', default='169.254.2.7')
    parser.add_argument('--elf', type=Path, default=Path(__file__).resolve().parents[1] / 'build_realtime/gd32e507_demo.elf')
    args = parser.parse_args()
    spec = importlib.util.spec_from_file_location('capture', args.frame_root / 'tests/hardware/realtime_scope_capture.py')
    capture = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(capture)
    dll_path = args.frame_root / 'build/native/Release/frame_backend.dll'
    os.add_dll_directory(str(dll_path.parent))
    dll = c.CDLL(str(dll_path))
    dll.frame_create.restype = c.c_void_p
    dll.frame_destroy.argtypes = [c.c_void_p]
    dll.frame_submit.argtypes = [c.c_void_p, c.c_char_p, c.c_uint32, c.POINTER(c.c_uint64)]
    dll.frame_result.argtypes = [c.c_void_p, c.c_uint64, c.c_void_p, c.c_uint32, c.POINTER(c.c_uint32)]
    dll.frame_release.argtypes = [c.c_void_p, c.c_uint64]
    common = capture.Backend(dll)
    data = None
    report = dict(passed=False, cycles=[], checks=[])
    output = args.elf.parent / 'hardware-behavior.json'

    def check(name, condition):
        report['checks'].append(dict(name=name, passed=bool(condition)))
        assert condition, name

    def control(action, **fields):
        return common.command('realtime-scope-control', action, **fields)

    def wait_status(predicate):
        for _ in range(50):
            status = control('status')
            if predicate(status):
                return status
            time.sleep(0.02)
        raise AssertionError(status)

    try:
        common.command('connect', transport='tcp', host=args.host, tcp_port=5000, dst=2, wire='e8')
        capability = control('capabilities')
        frequency = capability['sample_frequency']
        check('fixed existing interrupt queried at 10000 Hz', frequency == 10000)
        check('running firmware matches exact ELF', capability['elf_sha256'].lower() == hashlib.sha256(args.elf.read_bytes()).hexdigest())
        common.command('jlink', 'load', path=str(args.elf.resolve()))
        demo = common.command('jlink', 'expand', name='g_scope_demo', limit=32)
        wave = common.command('jlink', 'expand', name='g_scope_demo.wave', limit=32)
        symbols = {entry['name']: entry for entry in demo + wave}
        empty = control('configure', channels=[])
        check('empty synchronization still acknowledges rate', empty['sample_frequency'] == frequency)
        try:
            control('start')
        except RuntimeError:
            check('empty acquisition rejected', True)
        else:
            raise AssertionError('empty start accepted')

        for cycle in range(3):
            name = 'g_scope_demo.wave.' + ('cosine' if cycle == 1 else 'sine')
            # Deliberately send out of order; sampling must follow slot order.
            channels = [dict(channel=32, address=symbols['g_scope_demo.sample_count']['address'], type=6),
                        dict(channel=16, address=symbols['g_scope_demo.wave.square']['address'], type=6),
                        dict(channel=1, address=symbols[name]['address'], type=9)]
            ack = control('configure', channels=[])
            check(f'cycle {cycle}: clear ACK includes rate', ack['sample_frequency'] == frequency)
            for index, channel in enumerate(channels):
                ack = control('set-channel', channel=channel['channel'], channels=channels[:index + 1])
                check(f'cycle {cycle}: immediate CH{channel["channel"]} ACK includes rate', ack['sample_frequency'] == frequency)
            data = capture.Backend(dll)
            data.command('connect', transport='tcp', protocol='realtime-scope', host=args.host, tcp_port=9001)
            wait_status(lambda status: status['data_connected'])
            data.command('realtime-scope', 'configure', channels=channels)
            data.command('realtime-scope', 'start', frequency=frequency)
            check(f'cycle {cycle}: bank exchange start reports rate', control('start')['frequency'] == frequency)
            previous = None
            received = 0
            started = time.monotonic()
            while time.monotonic() - started < 2:
                batch = data.command('realtime-scope', 'drain')
                assert not batch['fault'] and not batch['missing'] and not batch['duplicates'] and not batch['rejected'], batch
                for row in batch['rows']:
                    signal, square, counter = row['values']
                    assert -2.001 <= signal <= 2.001 and square in (0, 1), row
                    assert previous is None or counter == (int(previous) + 1) & 0xffffffff, row
                    previous = counter
                received += len(batch['rows'])
            status = control('status')
            check(f'cycle {cycle}: ordered nested members, continuous real samples', received > 15000 and status['running'] and status['overflows'] == 0)
            report['cycles'].append(dict(cycle=cycle, received=received, status=status))
            if cycle == 1:
                data.close()
                data = None
                wait_status(lambda status: not status['running'] and not status['data_connected'])
                check('upload disconnect retires acquisition', True)
            else:
                control('stop')
                data.close()
                data = None
                wait_status(lambda status: not status['running'])
        # A receive-only data endpoint rejects peer input without harming public control.
        with socket.create_connection((args.host, 9001), timeout=3) as peer:
            wait_status(lambda status: status['data_connected'])
            peer.sendall(b'\x00')
            status = wait_status(lambda status: not status['data_connected'])
            check('data endpoint refuses incoming bytes', status['link_fault'] != 0)
        check('public control survives rejected upload input', control('capabilities')['sample_frequency'] == frequency)
        report['passed'] = True
    finally:
        try:
            control('stop')
        except RuntimeError:
            pass
        if data:
            data.close()
        common.close()
        output.write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps(report, ensure_ascii=False))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
