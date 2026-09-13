"""Plot actual PLECS Scope waveforms from saved RPC validation results."""
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'platform/plecs/npc/build/plot_deps'))
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

folder = ROOT / 'platform/plecs/npc/build/rpc_validation'
fig, axes = plt.subplots(3, 2, figsize=(13, 9), sharey=True)
for row, (load, title) in enumerate([
        ('light', 'Light: 1 W / 1 W / 1 W'),
        ('full', 'Full: 1 MW / 1 MW / 1 MW'),
        ('unbalanced', 'Unbalanced: 1 MW / 1 W / 1 W')]):
    for col, suffix in enumerate(['_baseline', '_directional_20s']):
        label = load + suffix
        data = np.loadtxt(folder / f'{label}_scope_50k.csv', delimiter=',', skiprows=1)
        metrics = json.loads((folder / f'{label}_scope_metrics.json').read_text(encoding='utf-8'))
        data = data[-2000:]
        for phase, color in enumerate(['#1769aa', '#d1495b', '#279567']):
            axes[row, col].plot((data[:, 0]-data[0, 0])*1000, data[:, phase+3],
                                color=color, lw=.8, label='ABC'[phase])
        rms = max(metrics['voltage_residual_rms'])
        axes[row, col].set_title(f'{title}\n{"Original DLL" if col == 0 else "Candidate DLL"}: residual {rms:.2f} V RMS', fontsize=10)
        axes[row, col].grid(alpha=.2)
        axes[row, col].set_xlim(0, 40)
        axes[row, col].set_ylim(-750, 750)
        axes[row, col].set_ylabel('Phase voltage (V)')
        axes[row, col].set_xlabel('Time within final two cycles (ms)')
axes[0, 0].legend(loc='upper right', ncol=3, fontsize=8)
fig.suptitle('Actual PLECS switched model | 5 kHz control | 10 us dead time', fontsize=14)
fig.text(.5, .01, 'Scope resampled at 50 kHz. Residual excludes DC and fitted 50 Hz fundamental; it is not standard THD.\nOriginal: 12 s runs. Candidate: 20 s runs. Remaining light-load ripple is visible.', ha='center', fontsize=9)
fig.tight_layout(rect=[0, .06, 1, .95])
target = ROOT / 'docs/design/assets/npc_actual_plecs_comparison.png'
fig.savefig(target, dpi=160)
print(target)
