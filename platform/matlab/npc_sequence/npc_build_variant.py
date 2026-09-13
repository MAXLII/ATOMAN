"""Build a recorded NPC parameter variant, then restore the exact source bytes.

Only explicitly listed replacements are allowed. A concurrent edit is never
overwritten during restoration. Variants are diagnostic DLLs, not releases.
"""
import hashlib
import json
from pathlib import Path
import subprocess
import sys

root = Path(__file__).resolve().parents[3]
variants = {
    'edge_current': ('code/interface/npc/common/pwm.c',
                     '    if (bsp_pwm_set_duty(duty) == false)',
                     '    candidate_edge_compensation(&modulator, duty);\n    if (bsp_pwm_set_duty(duty) == false)'),
    'matched_dt2': ('code/interface/npc/common/pwm.h',
                    '#define NPC_PWM_DEAD_TIME_S (0.00001f)',
                    '#define NPC_PWM_DEAD_TIME_S (0.000002f)'),
    'light_comp': ('code/interface/npc/common/pwm.h',
                   '#define NPC_PWM_COMPENSATION_CURRENT_MIN (200.0f)',
                   '#define NPC_PWM_COMPENSATION_CURRENT_MIN (0.0f)'),
    'hp_zero': ('code/ctrl/npc/npc_cfg.c', '.voltage_damping_gain = 0.15f', '.voltage_damping_gain = 0.0f'),
    'hp_double': ('code/ctrl/npc/npc_cfg.c', '.voltage_damping_gain = 0.15f', '.voltage_damping_gain = 0.3f'),
}
name = sys.argv[1]
relative, old, new = variants[name]
path = root / relative
original = path.read_bytes()
assert original.count(old.encode()) == 1
changed = original.replace(old.encode(), new.encode())
if name == 'edge_current':
    changed = changed.replace(b'#include "platform.h"', b'#include "platform.h"\n#include "npc_edge_compensation_candidate.h"')
build = root / 'platform/plecs/npc/build' / name
assert not build.exists(), 'Use a fresh variant name to prevent stale or loaded DLL reuse'
try:
    path.write_bytes(changed)
    subprocess.run(['cmake', '-S', str(root / 'platform/plecs/npc'), '-B', str(build),
                    '-G', 'MinGW Makefiles', '-DCMAKE_C_COMPILER=C:/mingw64/bin/gcc.exe',
                    '-DCMAKE_MAKE_PROGRAM=C:/mingw64/bin/mingw32-make.exe', '-DBUILD_TESTING=OFF',
                    '-DCMAKE_C_FLAGS=-I' + (root / 'platform/matlab/npc_sequence').as_posix()], check=True)
    subprocess.run(['cmake', '--build', str(build), '--target', 'plecs_npc', '--parallel', '4'], check=True)
    dll = build / 'bin/plecs_npc.dll'
    (build / 'variant.json').write_text(json.dumps({
        'source': relative, 'old': old, 'new': new,
        'original_sha256': hashlib.sha256(original).hexdigest(),
        'variant_sha256': hashlib.sha256(changed).hexdigest(),
        'dll_sha256': hashlib.sha256(dll.read_bytes()).hexdigest()}, indent=2), encoding='utf-8')
finally:
    if path.read_bytes() != changed:
        raise RuntimeError(f'Concurrent edit detected: {path}; manual comparison required')
    path.write_bytes(original)
print('Variant built; exact original source bytes restored:', name)
