"""Arm and observe an opt-in FAL test firmware; writes only its RAM command.

Build/download FAL_BOARD_TEST=1 first. The target task calls real APIs and confines
accepted writes to test partitions. SWD is observation/control, not function injection.
"""
import argparse
import hashlib
import json
import re
import subprocess
import time
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--elf', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--jlink', default='C:/Program Files/SEGGER/JLink_V936/JLink.exe')
    parser.add_argument('--serial', default='69409716')
    parser.add_argument('--confirm-test-partition-erase', action='store_true')
    args = parser.parse_args()
    if not args.confirm_test_partition_erase:
        parser.error('Explicit --confirm-test-partition-erase is required')
    symbols = subprocess.check_output(['arm-none-eabi-nm', str(args.elf)], text=True)
    match = re.search(r'^([0-9a-fA-F]+)\s+B\s+g_fal_board_report$', symbols, re.M)
    if not match:
        raise RuntimeError('ELF does not contain the hardware test report')
    address = int(match.group(1), 16)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open('w', encoding='utf-8') as log:
        def record(**fields):
            entry = dict(time=time.time(), **fields)
            text = json.dumps(entry, ensure_ascii=False)
            log.write(text + '\n')
            log.flush()
            print(text, flush=True)

        def commander(commands):
            result = subprocess.run([args.jlink, '-NoGui', '1', '-SelectEmuBySN', args.serial,
                                     '-device', 'GD32E507ZE', '-if', 'SWD', '-speed', '1000',
                                     '-ExitOnError', '1'], input='connect\n' + commands + '\nexit\n',
                                    capture_output=True, text=True, timeout=45)
            if result.returncode != 0 or '****** Error' in result.stdout:
                raise RuntimeError(result.stdout[-2000:] + result.stderr)
            return result.stdout

        def snapshot():
            output = commander(f'mem32 0x{address:X} 0x14')
            values = []
            for row in re.finditer(r'(?:J-Link>)?([0-9A-Fa-f]{8}) = ([0-9A-Fa-f ]+)', output):
                row_address = int(row.group(1), 16)
                if address <= row_address < address + 80:
                    values.extend(int(word,16) for word in row.group(2).split())
            if len(values) != 20:
                raise RuntimeError('Incomplete report: ' + output[-1000:])
            return dict(command=values[0], status=values[1], case=values[2], iteration=values[3],
                        passed=values[4:14], failure=values[14], expected=values[15], actual=values[16],
                        max_demo_us=values[17], corrected=values[18], bad_blocks=values[19])

        def run(command, cycle):
            initial = snapshot()
            if initial['status'] != 0:
                raise RuntimeError('Target must be freshly reset/unarmed: ' + str(initial))
            commander(f'w4 0x{address:X} 0x{command:X}')
            deadline = time.monotonic() + 900
            while time.monotonic() < deadline:
                time.sleep(5)
                state = snapshot()
                record(cycle=cycle, **state)
                if state['status'] == 3:
                    raise RuntimeError('Board assertion failed: ' + str(state))
                if state['status'] == 2:
                    wanted = {1:1, 2:100, 3:100, 4:100, 5:2, 6:2, 7:7, 8:1} if command == 1 else {1:1, 9:2}
                    if any(state['passed'][case] != count for case,count in wanted.items()):
                        raise RuntimeError('Incomplete case counters: ' + str(state))
                    if state['failure'] != 0 or state['bad_blocks'] != 0:
                        raise RuntimeError('Unexpected diagnostic: ' + str(state))
                    return state
            raise TimeoutError('Hardware suite exceeded 15 minutes')

        record(event='start', elf=str(args.elf), elf_sha256=hashlib.sha256(args.elf.read_bytes()).hexdigest(),
               report_address=hex(address), serial=args.serial)
        try:
            full = run(1, 0)
            reset_results = []
            for cycle in range(1,6):
                commander('r\ng')
                time.sleep(1)
                reset_results.append(run(2,cycle))
            record(event='summary', passed=True, full=full, reset_cycles=len(reset_results),
                   reset_test_transactions=sum(item['passed'][9] for item in reset_results))
        except Exception as error:
            record(event='failure', error=str(error))
            raise


if __name__ == '__main__':
    main()
