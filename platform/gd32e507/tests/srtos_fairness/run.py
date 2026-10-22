"""Measure two real 650 us tasks on GD32 and leave the test firmware running."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import time

PLATFORM = Path(__file__).resolve().parents[2]
JLINK = [r'C:\Program Files\SEGGER\JLink_V936\JLink.exe', '-SelectEmuBySN', '69409716',
         '-Device', 'GD32E507ZET6', '-If', 'SWD', '-Speed', '1000', '-AutoConnect', '1', '-ExitOnError', '1']
HEADER = 'magic target_us cpu_hz iterations calibrated_cycles discovered_tasks active_tasks boot_tick first_task error'.split()
TASK = 'entries completed context_errors min_wall_cycles max_wall_cycles last_tick'.split()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--seconds', type=float, default=5)
    parser.add_argument('--poll', type=float, default=2)
    parser.add_argument('--output')
    parser.add_argument('--restore-demo', action='store_true')
    parser.add_argument('--policy', choices=('fifo', 'new_first'), default='fifo')
    args = parser.parse_args()
    if args.seconds <= 0 or args.poll <= 0:
        parser.error('--seconds and --poll must be positive')
    output = PLATFORM / (args.output or f'build_fairness_{args.policy}_results')
    output.mkdir(exist_ok=True)
    report = dict(completed=False, seconds_per_order=args.seconds, rounds=[], firmware_restored=False,
                  test_firmware_running=False, policy=args.policy)

    def save():
        (output / 'result.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')

    def jlink(name, commands):
        path = output / (name + '.jlink')
        path.write_text(commands + '\nq\n', encoding='ascii')
        result = subprocess.run(JLINK + ['-CommandFile', str(path)], capture_output=True, text=True, timeout=90)
        (output / (name + '.log')).write_text(result.stdout + result.stderr, encoding='utf-8')
        if result.returncode:
            raise RuntimeError(result.stdout + result.stderr)
        return result.stdout

    def flash(name, root, target):
        log = jlink(name, f'r\nh\nloadfile {root / (target + ".hex")}\nverifybin {root / (target + ".bin")} 0x08000000\nr\ng')
        if 'Verify successful.' not in log:
            raise RuntimeError('Flash readback verification missing')

    try:
        for reverse in (0, 1):
            root = PLATFORM / (f'build_fairness_{reverse}' if args.policy == 'fifo'
                               else f'build_fairness_new_first_{reverse}')
            elf = root / 'gd32e507_fairness.elf'
            nm = subprocess.check_output(['arm-none-eabi-nm', '-S', str(elf)], text=True)
            symbols = {m[3]: (int(m[1], 16), int(m[2], 16)) for m in re.finditer(r'^(\w+) (\w+) \w (\w+)$', nm, re.M)}
            address, size = symbols['g_srtos_fairness']
            assert size == (len(HEADER) + 2 * len(TASK)) * 4
            tick_address = symbols['sys_tick_100us'][0]
            fault_address = symbols['g_section_fault_debug'][0]
            round_result = dict(reverse=reverse, elf_sha256=hashlib.sha256(elf.read_bytes()).hexdigest(), snapshots=[])
            report['rounds'].append(round_result)
            flash(f'flash_{reverse}', root, 'gd32e507_fairness')
            started = time.monotonic()
            sequence = 0

            def snapshot(halt=False):
                nonlocal sequence
                commands = ('h\n' if halt else '') + f'mem32 {address:X},{size // 4:X}\nmem32 {tick_address:X},1\n'
                if halt:
                    commands += f'mem32 {fault_address:X},1E\nmem32 E000ED28,2\n'
                log = jlink(f'order_{reverse}_{sequence:03}', commands)
                sequence += 1
                memory = {}
                for match in re.finditer(r'^([0-9A-F]{8}) = ((?:[0-9A-F]{8} ?)+)', log, re.M):
                    for i, word in enumerate(match[2].split()):
                        memory[int(match[1], 16) + 4 * i] = int(word, 16)
                raw = b''.join(memory[address + i].to_bytes(4, 'little') for i in range(0, size, 4))
                words = struct.unpack('<' + 'I' * (size // 4), raw)
                state = dict(zip(HEADER, words[:len(HEADER)]))
                for i, name in enumerate(('a', 'b')):
                    offset = len(HEADER) + i * len(TASK)
                    state[name] = dict(zip(TASK, words[offset:offset + len(TASK)]))
                state['elapsed_s'] = ((memory[tick_address] - state['boot_tick']) & 0xffffffff) / 10000
                state['calibrated_cpu_us'] = state['calibrated_cycles'] * 1e6 / max(1, state['cpu_hz'])
                a, b = state['a']['entries'], state['b']['entries']
                state['difference_pct_of_mean'] = abs(a - b) * 200 / max(1, a + b)
                if halt:
                    state['fault'] = {name: memory[fault_address + i * 4] for name, i in
                                      [('context_save_fail', 22), ('context_release_fail', 23), ('task_fault_reason', 24)]}
                    state['fault'].update(cfsr=memory[0xe000ed28], hfsr=memory[0xe000ed2c])
                assert state['magic'] == 0x46525032 and state['error'] == 0, state
                assert state['active_tasks'] == 2, state
                assert 600 <= state['calibrated_cpu_us'] <= 700, state
                assert state['a']['context_errors'] == state['b']['context_errors'] == 0, state
                round_result['snapshots'].append(state)
                save()
                print(json.dumps(dict(order='BA' if reverse else 'AB', elapsed_s=state['elapsed_s'],
                    a=a, b=b, difference_pct=round(state['difference_pct_of_mean'], 4),
                    calibrated_cpu_us=state['calibrated_cpu_us'], halted=halt)), flush=True)
                return state

            state = snapshot()
            while state['elapsed_s'] < args.seconds:
                time.sleep(min(args.poll, args.seconds - state['elapsed_s']))
                previous = state
                state = snapshot()
                assert state['a']['entries'] > previous['a']['entries'] and state['b']['entries'] > previous['b']['entries'], state
                if time.monotonic() - started > args.seconds + 60:
                    raise TimeoutError('Board test did not advance in time')
            final = snapshot(halt=True)
            assert all(value == 0 for value in final['fault'].values()), final
            round_result['final'] = final
            round_result['counts_within_one_percent'] = final['difference_pct_of_mean'] <= 1
            save()
        report['completed'] = True
    finally:
        try:
            if args.restore_demo:
                flash('restore_demo', PLATFORM / 'build_realtime', 'gd32e507_demo')
                report['firmware_restored'] = True
            else:
                jlink('resume_test', 'g')
                report['test_firmware_running'] = True
        finally:
            save()
    print(json.dumps(dict(completed=report['completed'], firmware_restored=report['firmware_restored'],
                         test_firmware_running=report['test_firmware_running'],
                         counts_close=[item.get('counts_within_one_percent') for item in report['rounds']])), flush=True)


if __name__ == '__main__':
    main()
