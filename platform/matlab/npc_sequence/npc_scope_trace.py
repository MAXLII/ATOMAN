"""Extract one scalar-signal PLECS 5 Scope SaveTraces record (format version 2).

RPC does not expose ExportCSV. This reader checks the saved stream's layout and
sample counts before using it; it does not assume the control-rate CSV is alias-free.
"""
import json
from pathlib import Path
import struct
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[3] / 'platform/plecs/npc/build/plot_deps'))
import numpy as np


def read_trace(path):
    with open(path, 'rb') as stream:
        def uint():
            return struct.unpack('>I', stream.read(4))[0]

        def string():
            size = uint()
            return None if size == 0xffffffff else stream.read(size).decode('utf-16-be')

        def series():
            metadata = struct.unpack('>3dI', stream.read(28))
            assert metadata[3] == 0, 'Only double-valued series are supported'
            count = uint()
            assert 0 < count < 100_000_000
            offset = stream.tell()
            stream.seek(count * 8, 1)
            return np.memmap(path, dtype='>f8', mode='r', offset=offset, shape=(count,))

        assert uint() == 2, 'Unsupported Scope format version'
        plots = []
        for _ in range(uint()):
            title = string()
            plots.append((title, [string() for _ in range(uint())]))
        assert uint() == 1, 'Expected one trace; preserve other traces separately'
        name = string()
        start, stop = struct.unpack('>2d', stream.read(16))
        for _, labels in plots:
            assert uint() == len(labels)
            assert all(uint() == 1 for _ in labels), 'Expected scalar channels'
        assert uint() == 1, 'Expected one time vector'
        time = series()
        values = []
        for _, labels in plots:
            assert uint() == len(labels)
            for label in labels:
                assert uint() == 1, 'Expected one series per scalar channel'
                value = series()
                assert len(value) == len(time)
                values.append((label, value))
        assert stream.tell() == Path(path).stat().st_size, 'Unexpected unparsed Scope data'
        assert np.all(np.diff(time) >= 0) and abs(time[-1]-stop) < 1e-7
        return name, start, stop, time, values


def analyze(label):
    root = Path(__file__).resolve().parents[3]
    output = root / 'platform/plecs/npc/build/rpc_validation'
    _, _, stop, time, values = read_trace(output / f'{label}.trace')
    uniform = stop - .2 + np.arange(10000) / 50000
    data = np.column_stack([np.interp(uniform, time, value) for _, value in values])
    np.savetxt(output / f'{label}_scope_50k.csv', np.column_stack([uniform, data]), delimiter=',',
               header='time,' + ','.join(name for name, _ in values), comments='')
    basis = np.column_stack([np.ones(len(uniform)), np.cos(2*np.pi*50*uniform), np.sin(2*np.pi*50*uniform)])
    voltage = data[:, 2:5]
    fit = np.linalg.lstsq(basis, voltage, rcond=None)[0]
    residual = voltage - basis @ fit
    a = np.exp(2j*np.pi/3)
    phasor = fit[1] - 1j*fit[2]
    pos = (phasor[0]+a*phasor[1]+a*a*phasor[2])/3
    neg = (phasor[0]+a*a*phasor[1]+a*phasor[2])/3
    rms = np.sqrt(np.mean(residual**2, axis=0))
    spectrum = np.fft.fft(residual, axis=0)/len(uniform)
    freq = abs(np.fft.fftfreq(len(uniform), 1/50000))
    results = {'label': label, 'stop': stop, 'original_samples': len(time),
               'phase_peak': np.hypot(fit[1], fit[2]).tolist(),
               'voltage_residual_rms': rms.tolist(),
               'voltage_below_2khz_rms': np.sqrt(np.sum(abs(spectrum[(freq>0)&(freq<=2000)])**2, axis=0)).tolist(),
               'positive_voltage_peak': abs(pos), 'negative_sequence_percent': 100*abs(neg)/abs(pos),
               'delta_mean': np.mean(data[:,0]-data[:,1]),
               'delta_peak': np.max(abs(data[:,0]-data[:,1]))}
    windows = []
    for beginning in np.arange(max(0.0, stop - 6.0), stop - .19, .2):
        grid = beginning + np.arange(10000) / 50000
        samples = np.column_stack([np.interp(grid, time, value) for _, value in values[2:5]])
        design = np.column_stack([np.ones(len(grid)), np.cos(2*np.pi*50*grid), np.sin(2*np.pi*50*grid)])
        coefficients = np.linalg.lstsq(design, samples, rcond=None)[0]
        remainder = samples - design @ coefficients
        windows.append([beginning, float(np.max(np.sqrt(np.mean(remainder**2, axis=0)))),
                        float(np.max(np.hypot(coefficients[1], coefficients[2])))])
    results['last_6s_windows'] = windows
    (output / f'{label}_scope_metrics.json').write_text(json.dumps(results, indent=2), encoding='utf-8')
    print(json.dumps(results, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    analyze(sys.argv[1])
