#!/usr/bin/env python3
"""Exercise every renderer family and inspect its actual floating-point WAV."""
import argparse
import json
from pathlib import Path
import subprocess
import numpy as np
from scipy.io import wavfile
p = argparse.ArgumentParser()
p.add_argument('renderer', type=Path)
p.add_argument('--output', type=Path)
a = p.parse_args()
renderer = a.renderer.resolve()
folder = renderer.parent / 'renders'
folder.mkdir(exist_ok=True)
results = []
for family in range(1, 33):
    path = folder / f'{family:02d}.wav'
    run = subprocess.run([str(renderer), '--family', str(family), '--output', str(path),
                          '--signal', 'music', '--seconds', '2', '--channels', '2',
                          '--mix', '1'], check=True, capture_output=True, text=True)
    metadata = json.loads(run.stdout)
    fs, x = wavfile.read(path)
    assert fs == 48000 and x.shape == (96000, 2) and x.dtype == np.float32
    assert np.isfinite(x).all() and np.max(np.abs(x)) > 0
    assert metadata['solver_failures'] == 0
    results.append(metadata)
report = {'status': 'passed', 'families': 32, 'signal': 'music',
          'duration_seconds': 2, 'results': results}
if a.output:
    a.output.write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps({'status': 'passed', 'wav_renders': len(results)}))
