"""Run the open NPC model without saving changes to its .plecs file.

Uses the existing FRAME CLI for parameter writes. RPC component edits are restored
in finally. Each run saves its transport responses and the DLL's 5 kHz CSV trace.
"""
import concurrent.futures
import hashlib
import json
from pathlib import Path
import socket
import subprocess
import sys
import time
import xmlrpc.client


def main():
    root = Path(__file__).resolve().parents[3]
    output = root / 'platform/plecs/npc/build/rpc_validation'
    output.mkdir(exist_ok=True)
    label = sys.argv[1] if len(sys.argv) > 1 else 'light'
    powers = {'light': [1, 1, 1], 'full': [1e6, 1e6, 1e6],
              'unbalanced': [1e6, 1, 1]}[label]
    dll = root / 'platform/plecs/npc/build/bin/plecs_npc.dll'
    if len(sys.argv) > 2 and sys.argv[2] == 'baseline':
        deployment = json.loads((output / 'release_deployment.json').read_text(encoding='utf-8'))
        dll = Path(deployment['backup'])
        label += '_baseline'
    elif len(sys.argv) <= 2 or sys.argv[2] == 'production':
        label += '_production'
    elif len(sys.argv) > 2 and sys.argv[2] == 'directional':
        dll = root / 'platform/plecs/npc/build/midpoint_directional/bin/plecs_npc.dll'
        label += '_directional'
    elif len(sys.argv) > 2:
        variant = sys.argv[2]
        assert variant in ['light_comp', 'hp_zero', 'hp_double', 'matched_dt2', 'edge_current', 'release_dt2']
        dll = root / 'platform/plecs/npc/build' / variant / 'bin/plecs_npc.dll'
        label += '_' + variant
    duration = float(sys.argv[3]) if len(sys.argv) > 3 else 12.0
    if len(sys.argv) > 3:
        label += f'_{duration:g}s'
    dead_time = float(sys.argv[4]) if len(sys.argv) > 4 else None
    if dead_time is not None:
        label += f'_dt{dead_time:g}'
    (output / f'{label}_binary.json').write_text(json.dumps({
        'dll': str(dll), 'sha256': hashlib.sha256(dll.read_bytes()).hexdigest(),
        'load_powers': powers, 'duration_s': duration, 'diagnostic_dead_time_s': dead_time}, indent=2), encoding='utf-8')
    server = xmlrpc.client.ServerProxy('http://localhost:1080')
    components = ['npc/DLL', 'npc/R2', 'npc/R3', 'npc/R4']
    original = {name: server.plecs.get(name) for name in components}
    dead_time_paths = [f'npc/{leg}/{pwm}/{delay}'
                       for leg in ['Sub', 'Sub1', 'Sub2']
                       for pwm in ['PWM', 'PWM1']
                       for delay in ['Turn-on Delay', 'Turn-on Delay1']]
    original_delays = {name: server.plecs.get(name)['T_d'] for name in dead_time_paths} if dead_time is not None else {}
    (output / f'{label}_settings.json').write_text(json.dumps(original, indent=2), encoding='utf-8')
    before = set(dll.parent.glob('npc_trace_*.csv'))
    frame = 'D:/OneDrive/LWX/FRAME/build/app/frame.exe'

    def simulate():
        rpc = xmlrpc.client.ServerProxy('http://localhost:1080')
        return rpc.plecs.simulate('npc', {'SolverOpts': {'TimeSpan': duration, 'Timeout': 300.0,
                                  'OutputTimes': [duration - .2, duration]}})

    try:
        filename = 'build\\bin\\plecs_npc.dll' if dll == root / 'platform/plecs/npc/build/bin/plecs_npc.dll' else dll.as_posix()
        server.plecs.set('npc/DLL', 'Filename', filename)
        for name in original_delays:
            server.plecs.set(name, 'T_d', f'{dead_time:.12g}')
        for name, power in zip(components[1:], powers):
            server.plecs.set(name, 'R', f'34.5e3*34.5e3/{power:.12g}')
        with concurrent.futures.ThreadPoolExecutor(max_workers=1) as pool:
            started = time.monotonic()
            simulation = pool.submit(simulate)
            for attempt in range(100):
                try:
                    with socket.create_connection(('127.0.0.1', 5000), timeout=.1):
                        break
                except OSError:
                    if simulation.done():
                        simulation.result()
                        raise RuntimeError('Simulation ended before FRAME transport became available')
                    time.sleep(.05)
            for name, value in [('VD_POS_REF', '563'), ('RUN_ENABLE', '1')]:
                args = [frame, 'param', 'write', '--name', name, '--value', value,
                        '--transport', 'tcp', '--host', '127.0.0.1', '--tcp-port', '5000',
                        '--dst', '2', '--response-timeout', '2000', '--json']
                result = subprocess.run(args, capture_output=True, timeout=15)
                text = result.stdout.decode('utf-8', errors='replace')
                (output / f'{label}_{name}.json').write_text(text, encoding='utf-8')
                print(name, result.returncode, text[:700], flush=True)
                if result.returncode != 0:
                    raise RuntimeError(result.stderr.decode('utf-8', errors='replace'))
            simulation.result()
            print(label, 'completed in', round(time.monotonic()-started, 3), 'wall seconds', flush=True)
        traces = sorted(set(dll.parent.glob('npc_trace_*.csv')) - before)
        (output / f'{label}_traces.json').write_text(json.dumps([str(p) for p in traces]), encoding='utf-8')
        print('Trace files:', [p.name for p in traces], flush=True)
        server.plecs.scope('npc/Scope', 'SaveTraces', (output / f'{label}.trace').as_posix())
    finally:
        server.plecs.set('npc/DLL', 'Filename', original['npc/DLL']['Filename'])
        for name in components[1:]:
            server.plecs.set(name, 'R', original[name]['R'])
        for name, value in original_delays.items():
            server.plecs.set(name, 'T_d', value)
        print('Restored original in-memory DLL and load parameters', flush=True)


if __name__ == '__main__':
    main()
