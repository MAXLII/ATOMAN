"""ESP32 physical scope/SRTOS acceptance through FRAME's production backend.

No emulated transport or signal is provided. The final channel always samples the
real interrupt counter. Use --profile-cycles (at least four channels) to include
the timer's cycle stamp, accumulated ISR cost, and maximum ISR cost. Parameter
observations use registered names, not hard-coded RAM addresses or layouts.
"""
import argparse
from collections import deque
import ctypes as c
import hashlib
import importlib.util
import json
import math
import os
from pathlib import Path
import re
import socket
import struct
import threading
import time


def load_backend(frame_root):
    helper = frame_root / 'tests/hardware/realtime_scope_capture.py'
    spec = importlib.util.spec_from_file_location('frame_scope_capture', helper)
    capture = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(capture)
    dll_path = frame_root / 'build/native/Release/frame_backend.dll'
    directory = os.add_dll_directory(str(dll_path.parent))
    dll = c.CDLL(str(dll_path))
    dll.frame_create.restype = c.c_void_p
    dll.frame_destroy.argtypes = [c.c_void_p]
    dll.frame_submit.argtypes = [c.c_void_p, c.c_char_p, c.c_uint32, c.POINTER(c.c_uint64)]
    dll.frame_result.argtypes = [c.c_void_p, c.c_uint64, c.c_void_p, c.c_uint32, c.POINTER(c.c_uint32)]
    dll.frame_release.argtypes = [c.c_void_p, c.c_uint64]
    return capture.Backend, dll, directory


class RawFrames:
    """Fixed channel-layout EA validation; arbitrary TCP read boundaries are allowed."""

    def __init__(self, channels):
        formats = {1: 'b', 2: 'B', 3: 'h', 4: 'H', 5: 'i', 6: 'I',
                   7: 'q', 8: 'Q', 9: 'f', 10: 'd', 11: '?'}
        self.layout = struct.Struct('<' + ''.join(formats[row['type']] for row in
                                                   sorted(channels, key=lambda row: row['channel'])))
        self.frame_size = self.layout.size + 5
        self.buffer = bytearray()
        self.previous_index = None
        self.next_index = self.missing = self.duplicates = self.rejected = 0

    def feed(self, data):
        self.buffer.extend(data)
        rows = []
        consumed = 0
        while len(self.buffer) - consumed >= self.frame_size:
            frame = self.buffer[consumed:consumed + self.frame_size]
            consumed += self.frame_size
            if (frame[0] != 0xea or int.from_bytes(frame[-3:-1], 'little') != self.layout.size or
                    (sum(frame[:-1]) & 255) != frame[-1]):
                self.rejected += 1
                raise ValueError('Raw EA header, payload length or checksum mismatch: ' + frame.hex())
            delta = 1 if self.previous_index is None else (frame[1] - self.previous_index) & 255
            if delta == 0:
                self.duplicates += 1
                continue
            self.missing += delta - 1
            self.next_index += delta
            self.previous_index = frame[1]
            rows.append(dict(index=self.next_index - 1, values=self.layout.unpack_from(frame, 2)))
        del self.buffer[:consumed]
        return rows


class RawReceiver:
    """Receive/validate in a separate thread, independent of E8 control queries.

    Uses the same requested socket receive capacity and decoded-row limit as the
    FRAME native path. This is a real TCP receiver, never a packet generator.
    """

    def __init__(self, host, port, channels):
        self.decoder = RawFrames(channels)
        self.rows = deque()
        self.lock = threading.Lock()
        self.closed = threading.Event()
        self.expected_stop = threading.Event()
        self.fault = ''
        self.bytes_received = self.read_calls = self.queue_peak = 0
        self.max_read_interval_us = 0.0
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:
            self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 1024 * 1024)
            self.socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
            self.socket.settimeout(3)
            self.socket.connect((host, port))
            self.socket.settimeout(0.05)
            self.receive_capacity = self.socket.getsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF)
            self.worker = threading.Thread(target=self.receive, name='scope-raw-receiver', daemon=True)
            self.worker.start()
        except Exception:
            self.socket.close()
            raise

    def receive(self):
        previous_read = None
        try:
            while not self.closed.is_set():
                try:
                    block = self.socket.recv(65536)
                except socket.timeout:
                    continue
                if not block:
                    if self.expected_stop.is_set():
                        return
                    raise ConnectionError('Raw data endpoint closed during acquisition')
                now = time.monotonic()
                if previous_read is not None:
                    self.max_read_interval_us = max(self.max_read_interval_us, (now - previous_read) * 1000000)
                previous_read = now
                self.bytes_received += len(block)
                self.read_calls += 1
                rows = self.decoder.feed(block)
                with self.lock:
                    if len(self.rows) + len(rows) > 32768:
                        raise BufferError('Raw decoded-row queue exceeded 32768 samples')
                    self.rows.extend(rows)
                    self.queue_peak = max(self.queue_peak, len(self.rows))
        except Exception as error:
            if not self.closed.is_set():
                with self.lock:
                    self.fault = repr(error)

    def drain(self):
        with self.lock:
            rows = [self.rows.popleft() for _ in range(min(len(self.rows), 8192))]
            return dict(rows=rows, missing=self.decoder.missing, duplicates=self.decoder.duplicates,
                        rejected=self.decoder.rejected, pending=len(self.rows), fault=self.fault)

    def close(self):
        self.closed.set()
        try:
            self.socket.shutdown(socket.SHUT_RDWR)
        except OSError:
            pass
        self.socket.close()
        self.worker.join(timeout=2)
        if self.worker.is_alive():
            raise RuntimeError('Raw receiver thread did not stop')

    def diagnostics(self):
        return dict(bytes_received=self.bytes_received, read_calls=self.read_calls,
                    socket_receive_capacity=self.receive_capacity, decoded_queue_peak=self.queue_peak,
                    max_read_interval_us=self.max_read_interval_us, final_partial_frame_bytes=len(self.decoder.buffer))


class Samples:
    """Bounded observations; never retain the whole acquisition in Python."""

    def __init__(self, profile, observe_overdue=False):
        self.profile = profile
        self.observe_overdue = observe_overdue
        self.first_overdue = self.last_overdue = None
        self.received = self.counter_gaps = self.cycle_intervals = 0
        self.cycle_total = self.busy_total = self.max_cycle_gap = self.max_body = 0
        self.previous_counter = self.previous_cycle = self.previous_busy = None
        self.first_counter = self.last_counter = None
        self.first_gap = None
        self.pending_peak = 0
        self.errors = dict(missing=0, duplicates=0, rejected=0)
        self.fault = ''

    def accept(self, batch):
        for name in self.errors:
            self.errors[name] = max(self.errors[name], batch[name])
        self.fault = batch.get('fault') or self.fault
        self.pending_peak = max(self.pending_peak, batch.get('pending', 0))
        rows = batch['rows']
        self.received += len(rows)
        for row in rows:
            values = row['values']
            counter = int(values[-1])
            if self.previous_counter is not None and counter != (self.previous_counter + 1) & 0xffffffff:
                self.counter_gaps += 1
                if self.first_gap is None:
                    self.first_gap = dict(index=row['index'], previous=self.previous_counter, current=counter)
            if self.first_counter is None:
                self.first_counter = counter
            self.last_counter = self.previous_counter = counter
            if self.profile:
                cycle, busy, body = (int(value) for value in values[-4:-1])
                if self.previous_cycle is not None:
                    delta = (cycle - self.previous_cycle) & 0xffffffff
                    self.cycle_total += delta
                    self.cycle_intervals += 1
                    self.max_cycle_gap = max(self.max_cycle_gap, delta)
                    self.busy_total += (busy - self.previous_busy) & 0xffffffff
                self.previous_cycle, self.previous_busy = cycle, busy
                self.max_body = max(self.max_body, body)
                if self.observe_overdue:
                    overdue = int(values[-5])
                    if self.first_overdue is None:
                        self.first_overdue = overdue
                    self.last_overdue = overdue
        return len(rows)

    def result(self, cpu_mhz):
        result = dict(received=self.received, counter_gaps=self.counter_gaps,
                      first_counter=self.first_counter, last_counter=self.last_counter,
                      first_counter_gap=self.first_gap, decoder_pending_peak=self.pending_peak,
                      fault=self.fault, **self.errors)
        if self.cycle_total:
            result.update(hardware_hz=self.cycle_intervals * cpu_mhz * 1000000 / self.cycle_total,
                          hardware_mean_interval_us=self.cycle_total / self.cycle_intervals / cpu_mhz,
                          hardware_max_interval_us=self.max_cycle_gap / cpu_mhz,
                          isr_cpu_percent=self.busy_total * 100 / self.cycle_total,
                          isr_mean_us=self.busy_total / self.cycle_intervals / cpu_mhz,
                          isr_max_us=self.max_body / cpu_mhz,
                          isr_max_scope='since firmware timer initialization',
                          cpu_frequency_mhz=cpu_mhz)
        if self.first_overdue is not None:
            result.update(hardware_overdue_before=self.first_overdue,
                          hardware_overdue_after=self.last_overdue,
                          hardware_overdue_events=(self.last_overdue - self.first_overdue) & 0xffffffff)
        return result


def arguments():
    platform = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--frame-root', type=Path, default=Path('D:/OneDrive/LWX/FRAME'))
    parser.add_argument('--elf', type=Path, default=platform / 'build/esp32c5_srtos_demo.elf')
    parser.add_argument('--host', default='192.168.1.17')
    parser.add_argument('--port', default='COM7', help='Public E8 serial control port')
    parser.add_argument('--baud', type=int, default=115200)
    parser.add_argument('--control-tcp', action='store_true', help='Use public E8 TCP control instead of serial')
    parser.add_argument('--control-port', type=int, default=5000)
    parser.add_argument('--data-port', type=int, default=9001, help='Independent upload-only TCP port')
    parser.add_argument('--channels', type=int, choices=range(1, 33), default=8, metavar='1..32')
    parser.add_argument('--seconds', type=float, default=30)
    parser.add_argument('--expected-frequency', type=int, default=31250)
    parser.add_argument('--profile-cycles', action='store_true')
    parser.add_argument('--raw-receiver', action='store_true',
                        help='Receive real EA TCP packets directly; retain FRAME E8 control and ELF checks')
    parser.add_argument('--cpu-mhz', type=float, default=240, help='Actual firmware CPU clock, for cycle conversion only')
    parser.add_argument('--observe', action='append', default=[], metavar='PARAMETER', help='Required registered parameter name; repeatable')
    parser.add_argument('--observe-pattern', default='(?i)srtos|fair', help='Additional optional parameter names selected from the directory')
    parser.add_argument('--write', action='append', default=[], metavar='NAME=VALUE',
                        help='Write a registered numeric parameter before acquisition; repeatable')
    parser.add_argument('--output', type=Path, default=platform / 'build/srtos-scope-acceptance')
    args = parser.parse_args()
    if (not math.isfinite(args.seconds) or not math.isfinite(args.cpu_mhz) or
            args.seconds <= 0 or args.cpu_mhz <= 0 or args.expected_frequency <= 0):
        parser.error('seconds, cpu-mhz and expected-frequency must be positive')
    if args.profile_cycles and args.channels < 4:
        parser.error('--profile-cycles requires at least four channels')
    try:
        re.compile(args.observe_pattern)
    except re.error as error:
        parser.error(str(error))
    writes = []
    for item in args.write:
        name, separator, value = item.partition('=')
        if not name or not separator:
            parser.error('--write requires NAME=VALUE')
        try:
            number = json.loads(value)
        except ValueError:
            parser.error('--write VALUE must be a JSON number')
        if isinstance(number, bool) or not isinstance(number, (int, float)) or not math.isfinite(number):
            parser.error('--write VALUE must be a finite number')
        writes.append((name, number))
    args.writes = writes
    return args


def run(args):
    args.output.mkdir(parents=True, exist_ok=True)
    report = dict(passed=False, channels=args.channels, requested_seconds=args.seconds,
                  elf=str(args.elf.resolve()), elf_sha256=hashlib.sha256(args.elf.read_bytes()).hexdigest(),
                  control_transport='tcp' if args.control_tcp else 'serial', host=args.host,
                  profile_cycles=args.profile_cycles,
                  receiver='raw-tcp' if args.raw_receiver else 'frame-native', checks=[])
    Backend, dll, directory = load_backend(args.frame_root.resolve())
    common = data = raw = None
    observed = []
    samples = Samples(args.profile_cycles, args.profile_cycles and args.channels >= 5)
    events = (args.output / 'events.jsonl').open('w', encoding='utf-8')
    recording = False

    def record(event, value):
        events.write(json.dumps(dict(event=event, monotonic=time.monotonic(), data=value)) + '\n')
        events.flush()

    def check(name, condition):
        report['checks'].append(dict(name=name, passed=bool(condition)))
        if not condition:
            raise AssertionError(name)

    def control(action, **fields):
        return common.command('realtime-scope-control', action, **fields)

    def wifi_snapshot(label):
        response = common.command('serial', 'raw', text='wifi\r\n', duration=0.4)
        text = ''.join(bytes.fromhex(row['hex']).decode('utf-8', 'replace')
                       for row in response['records'] if row['dir'] == 'rx')
        report[label] = dict(text=text, values={name: int(value) for name, value in re.findall(r'\b([a-z_]+)=(-?\d+)\b', text)})
        record(label, report[label])
        (args.output / (label + '.txt')).write_text(text, encoding='utf-8')
        check(label + ': status command answered', 'wifi ' in text)

    def parameter_snapshot(label):
        report[label] = {name: common.command('param', 'read', name=name) for name in observed}
        record(label, report[label])
        for name in ('SRTOS_FAULT', 'SRTOS_INVARIANTS', 'SRTOS_ERRORS'):
            if name in report[label]:
                check(label + ': ' + name + ' is zero', report[label][name]['value'] == 0)

    try:
        common = Backend(dll)
        if args.control_tcp:
            common.command('connect', transport='tcp', host=args.host, tcp_port=args.control_port, dst=2, wire='e8')
        else:
            common.command('connect', transport='serial', port=args.port, baud=args.baud, dst=2, wire='e8')
        control('stop')
        capability = control('capabilities')
        report['capabilities'] = capability
        check('running firmware matches exact ELF', capability['elf_sha256'].lower() == report['elf_sha256'])
        frequency = capability['sample_frequency']
        check('queried fixed sampling frequency', frequency == args.expected_frequency)
        wifi_snapshot('wifi_before')
        parameters = common.command('param', 'list')
        available = {row['name'] for row in parameters}
        required = set(args.observe) | {name for name, _ in args.writes} | {'SRTOS_FAULT', 'SRTOS_INVARIANTS'}
        check('required SRTOS observation parameters exist', required <= available)
        observed = sorted(required | ({'SRTOS_ERRORS'} & available) |
                          {name for name in available if re.search(args.observe_pattern, name)})
        report['parameter_writes'] = [common.command('param', 'write', name=name, value=value)
                                      for name, value in args.writes]
        parameter_snapshot('parameters_before')

        # jlink/load and expand inspect ELF metadata; no J-Link or debug probe is opened.
        common.command('jlink', 'load', path=str(args.elf.resolve()))
        symbols = {}
        for name in ('g_scope_demo', 'g_scope_demo.wave') + (('g_bsp_timer_debug',) if args.profile_cycles else ()):
            symbols.update((row['name'], row) for row in common.command('jlink', 'expand', name=name, limit=32))
        wave_names = ['g_scope_demo.wave.' + name for name in ('sine', 'cosine', 'triangle', 'square')]
        names = [wave_names[index % 4] for index in range(args.channels - 1)] + ['g_scope_demo.sample_count']
        if args.profile_cycles:
            names[-4:-1] = ['g_bsp_timer_debug.' + name for name in ('cycle_stamp', 'busy_cycles', 'max_body_cycles')]
            if args.channels >= 5:
                names[-5] = 'g_bsp_timer_debug.overdue'
        channels = [dict(channel=index + 1, address=symbols[name]['address'], type=9 if name in wave_names[:3] else 6)
                    for index, name in enumerate(names)]
        report['channel_names'] = names
        report['channel_configuration'] = channels
        ack = control('configure', channels=channels)
        check('configuration ACK returns fixed rate', ack['sample_frequency'] == frequency)
        if args.raw_receiver:
            raw = RawReceiver(args.host, args.data_port, channels)
        else:
            data = Backend(dll)
            data.command('connect', transport='tcp', protocol='realtime-scope', host=args.host, tcp_port=args.data_port)
        deadline = time.monotonic() + 3
        while True:
            status = control('status')
            if status['data_connected'] or time.monotonic() >= deadline:
                break
            time.sleep(0.02)
        check('upload-only endpoint connected', status['data_connected'])
        if data is not None:
            data.command('realtime-scope', 'configure', channels=channels)
            data.command('realtime-scope', 'start', frequency=frequency)
        ack = control('start')
        recording = True
        check('start ACK retains fixed rate', ack['frequency'] == frequency)
        started = time.monotonic()
        next_status = started
        queue_peak = 0
        while time.monotonic() - started < args.seconds:
            received = samples.accept(raw.drain() if raw is not None else data.command('realtime-scope', 'drain'))
            if raw is not None and received == 0:
                time.sleep(0.001)
            if samples.fault or any(samples.errors.values()) or samples.counter_gaps:
                raise AssertionError('decoder or real sample-counter continuity failed')
            now = time.monotonic()
            if now >= next_status:
                status = control('status')
                report['device_during'] = status
                queue_peak = max(queue_peak, status.get('queue_high_water', status['queue_count']))
                record('status', dict(status, received=samples.received))
                check('acquisition remains running without a device fault', status['running'] and
                      not status['overflows'] and not status['timing_fault'] and not status.get('link_fault'))
                next_status = now + 0.5
        elapsed = time.monotonic() - started
        measured_received = samples.received
        report.update(elapsed_s=elapsed, measured_received=measured_received,
                      receive_hz=measured_received / elapsed, frame_bytes=4 * args.channels + 5,
                      wire_bytes_per_s=measured_received * (4 * args.channels + 5) / elapsed,
                      payload_bytes_per_s=measured_received * 4 * args.channels / elapsed,
                      queue_peak_frames=queue_peak, queue_peak_bytes=queue_peak * (4 * args.channels + 5))
        report['device_before_stop'] = control('status')
        queue_peak = max(queue_peak, report['device_before_stop'].get('queue_high_water', 0))
        report['queue_peak_frames'] = queue_peak
        report['queue_peak_bytes'] = queue_peak * report['frame_bytes']
        report['expected_wire_bytes_per_s'] = frequency * report['frame_bytes']
        if raw is not None:
            raw.expected_stop.set()
        report['stop_ack'] = control('stop')
        recording = False
        # The upload endpoint may close on an intentional stop. The final in-run
        # drain is the continuity boundary; do not query a now-closed data link.
        report['device_after_stop'] = control('status')
        parameter_snapshot('parameters_after')
        report['parameter_deltas'] = {
            name: (int(report['parameters_after'][name]['value']) -
                   int(report['parameters_before'][name]['value'])) & 0xffffffff
            for name in observed
            if isinstance(report['parameters_before'][name]['value'], int) and
            isinstance(report['parameters_after'][name]['value'], int)}
        wifi_snapshot('wifi_after')
        report.update(samples.result(args.cpu_mhz))
        report['continuity_scope'] = ('EA and sample_count continuity covers received frames only; '
                                      'it does not prove that every hardware timer expiration was sampled.')
        if args.profile_cycles:
            check('hardware cycle intervals were captured', 'hardware_hz' in report)
            report['hardware_frequency_error_percent'] = abs(report['hardware_hz'] - frequency) * 100 / frequency
            check('hardware sample rate differs from queried rate by less than 0.1 percent',
                  report['hardware_frequency_error_percent'] < 0.1)
        check('elapsed duration completed', elapsed >= args.seconds)
        check('received at least 90 percent of expected sample rate', report['receive_hz'] >= frequency * 0.9)
        check('EA sequence, length and checksum accepted by selected receiver', not any(samples.errors.values()) and not samples.fault)
        check('every captured interrupt counter is consecutive', samples.received > 1 and samples.counter_gaps == 0)
        status = report['device_before_stop']
        check('final in-run status healthy', status['running'] and not status['overflows'] and
              not status['timing_fault'] and not status.get('link_fault'))
        check('requested stop acknowledged', not report['device_after_stop']['running'])
        report['passed'] = True
    except Exception as error:
        report['error'] = repr(error)
        record('error', report['error'])
    finally:
        if common is not None:
            if recording:
                try:
                    report['failure_status'] = control('status')
                except Exception as error:
                    report['failure_status_error'] = repr(error)
            try:
                if raw is not None:
                    raw.expected_stop.set()
                control('stop')
            except Exception as error:
                report['cleanup_stop_error'] = repr(error)
                report['passed'] = False
            if 'wifi_after' not in report:
                try:
                    wifi_snapshot('wifi_after')
                except Exception as error:
                    report['wifi_after_error'] = repr(error)
            if observed and 'parameters_after' not in report:
                try:
                    parameter_snapshot('parameters_after')
                except Exception as error:
                    report['parameters_after_error'] = repr(error)
        if data is not None:
            data.close()
        if raw is not None:
            raw.close()
            report['raw_receiver'] = raw.diagnostics()
        if common is not None:
            common.close()
        directory.close()
        events.close()
        report.update(samples.result(args.cpu_mhz))
        (args.output / 'result.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps(report, ensure_ascii=False))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(run(arguments()))
