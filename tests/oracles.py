#!/usr/bin/env python3
"""Compare compiled C++ outputs with independently derived NumPy/SciPy references."""
from pathlib import Path
import argparse
import io
import json
import platform
import subprocess
import tempfile
import numpy as np
import scipy
from scipy import signal, linalg, optimize

parser = argparse.ArgumentParser()
parser.add_argument('renderer', type=Path)
parser.add_argument('--output', type=Path)
args = parser.parse_args()
renderer = args.renderer.resolve()
checks = []
rng = np.random.default_rng(74031)
fs = 48000


def verify(name, actual, reference, limit=1e-9, scope=''):
    error = float(np.max(np.abs(actual-reference)))
    checks.append(dict(name=name, max_absolute_error=error, limit=limit, scope=scope, passed=error <= limit))
    if error > limit:
        raise AssertionError(f'{name}: {error} > {limit}')


with tempfile.TemporaryDirectory() as temp:
    root = Path(temp)
    def render(family, x, **parameters):
        source = root/'input.txt'
        np.savetxt(source, x, fmt='%.17g')
        command = [str(renderer), '--family', family, '--input', str(source), '--raw', '--mix', '1', '--block', '127']
        for key, value in parameters.items():
            command.extend(['--'+key.replace('_', '-'), str(value)])
        result = subprocess.run(command, check=True, capture_output=True, text=True)
        return np.loadtxt(io.StringIO(result.stdout))

    x = rng.normal(0, .1, 2053)
    # RBJ transfer-function oracle, independently evaluated by scipy lfilter.
    for sr in (8000, 44100, 48000, 96000, 192000):
        f, q, db = min(1000., sr*.1), .9, 12
        w, A = 2*np.pi*f/sr, 10**(db/40)
        alpha = np.sin(w)/(2*q)
        b = [1+alpha*A, -2*np.cos(w), 1-alpha*A]
        a = [1+alpha/A, -2*np.cos(w), 1-alpha/A]
        verify(f'EQ at {sr} Hz', render('eq', x, sample_rate=sr, frequency=f, q=q, gain_db=db),
               signal.lfilter(b, a, x), 1e-10)

    omega = 2*fs*np.tan(np.pi*1200/fs)
    b, a = signal.bilinear([omega, 0], [1, omega/.8, omega**2], fs)
    verify('SVF band-pass', render('wah', x, frequency=1200, q=.8), signal.lfilter(b, a, x), 1e-10)

    reference = np.zeros_like(x)
    for n in range(len(x)):
        if n >= 48:
            reference[n] = x[n-48]+.3*reference[n-48]
    verify('Feedback delay', render('delay', x, delay=.001, feedback=.3), reference, 1e-14)
    verify('Zero-depth chorus', render('chorus', x, depth=0), np.r_[np.zeros(864), x[:-864]], 1e-12)
    verify('Through-zero matched paths', render('flanger', x, depth=0, feedback=0), np.r_[np.zeros(960), x[:-960]], 1e-14)
    verify('Tremolo gain law', render('tremolo', x, depth=.05, rate=1), x*(.5+.5*np.sin(2*np.pi*np.arange(len(x))/fs)), 1e-13)
    verify('Ring multiplication', render('ring', x, frequency=500), x*np.cos(2*np.pi*500*np.arange(len(x))/fs), 1e-12)

    impulse = np.r_[1., np.zeros(8191)]
    actual = render('bbd', impulse, delay=.01, feedback=0)
    first = int(np.flatnonzero(np.abs(actual)>1e-15)[0])
    verify('BBD clock arrival', np.array([first]), np.array([481]), 0, 'Discrete clock convention, event at end of audio sample')

    for length in (1, 127, 128, 129, 301, 1027):
        ir = rng.normal(0, .03, length)
        kernel = root/'ir.txt'
        np.savetxt(kernel, ir, fmt='%.17g')
        padded = np.r_[x, np.zeros(length+128)]
        actual = render('convolution', padded, ir=kernel)
        expected = np.r_[np.zeros(128), np.convolve(padded, ir)][:len(padded)]
        verify(f'Partitioned convolution {length} taps', actual, expected, 1e-12)

    padded = np.r_[x, np.zeros(512)]
    verify('Streaming STFT identity and tail', render('spectral', padded), np.r_[np.zeros(256), padded[:-256]], 1e-12)
    verify('Unity-ratio spectral pitch', render('pitch', padded, pitch=0), np.r_[np.zeros(1024), padded[:-1024]], 1e-12)

    u = .8*np.sin(2*np.pi*997*np.arange(1024)/fs)
    reference = []
    old = 0.
    for value in u:
        residual = lambda v: 10e-9*fs*(v-old)+(v-value)/2200+2e-9*np.sinh(v/.026)
        old = optimize.brentq(residual, min(old, value, 0)-1e-9, max(old, value, 0)+1e-9, xtol=1e-14)
        reference.append(old)
    verify('Implicit diode versus Brent', render('diode', u), np.asarray(reference), 2e-9)

    # Oversampled circuit: explicit zero stuffing and scipy FIR filtering independently.
    for K in (2, 4, 8):
        length = 16*K+1
        k = np.arange(length)
        window = .42-.5*np.cos(2*np.pi*k/(length-1))+.08*np.cos(4*np.pi*k/(length-1))
        taps = .9/K*np.sinc(.9/K*(k-(length-1)/2))*window
        taps /= taps.sum()
        stuffed = np.zeros(len(u)*K)
        stuffed[::K] = u
        interpolated = signal.lfilter(K*taps, [1], stuffed)
        state, processed = 0., []
        for value in interpolated:
            residual = lambda v: 10e-9*fs*K*(v-state)+(v-value)/2200+2e-9*np.sinh(v/.026)
            state = optimize.brentq(residual, min(state, value, 0)-1e-9, max(state, value, 0)+1e-9, xtol=1e-14)
            processed.append(state)
        expected = signal.lfilter(taps, [1], processed)[::K]
        verify(f'Diode with {K}x filtered oversampling', render('diode', u, oversampling=K), expected, 3e-9)

    # Linear speaker: augmented exact ZOH reference through scipy's matrix exponential.
    A = np.array([[0, 1, 0], [-1200/.02, -1.2/.02, 4/.02], [0, -4/.0006, -6/.0006]])
    augmented = np.zeros((4, 4))
    augmented[:3, :3] = A
    augmented[2, 3] = 1/.0006
    discrete = linalg.expm(augmented/fs)
    state = np.zeros(3)
    expected = []
    for value in x:
        state = discrete[:3, :3]@state+discrete[:3, 3]*value
        expected.append(state[1])
    verify('Physical speaker exact ZOH', render('speaker', x), np.asarray(expected), 1e-10)

    # Plate: independent closed-form step-response difference for an impulse held one sample.
    h, rho, E, nu, lx, ly = .0005, 7800, 2e11, .3, 2., 1.
    D, mass = E*h**3/(12*(1-nu*nu)), rho*h*lx*ly/4
    candidates = []
    for m in range(1, 33):
        for n in range(1, 33):
            k2 = (m*np.pi/lx)**2+(n*np.pi/ly)**2
            w = np.sqrt(D*k2*k2/(rho*h))
            weight = np.sin(m*np.pi*.23)*np.sin(n*np.pi*.37)/mass*np.sin(m*np.pi*.61)*np.sin(n*np.pi*.41)*10000
            candidates.append((w, weight))
    time = (np.arange(2048)+1)/fs
    sigma = np.log(1000)/1.5
    expected = np.zeros(len(time))
    for w, weight in sorted(candidates, key=lambda c: c[0])[:64]:
        wd = np.sqrt(w*w-sigma*sigma)
        step = lambda t: (1-np.exp(-sigma*t)*(np.cos(wd*t)+sigma/wd*np.sin(wd*t)))/(w*w)
        expected += weight*(step(time)-step(np.maximum(0, time-1/fs)))
    verify('Physical plate closed-form modes', render('plate', np.r_[1., np.zeros(2047)]), expected, 2e-9,
           '64 simply supported modes, physical modal mass, held force input')

    # Compressor DC static curve after settling; unnormalised amplitude is retained.
    actual = render('compressor', np.full(24000, .5), threshold_db=-18, ratio=4)
    target_db = -18+(20*np.log10(.5)+18)/4
    verify('Compressor settled static gain', actual[-100:], np.full(100, 10**(target_db/20)), 1e-9)

    # Frequency-shift suppression and pitch ratio tested in coherent steady-state spectra.
    tone = np.sin(2*np.pi*2000*np.arange(48000)/fs)*.1
    shifted = render('frequency-shift', tone, frequency=500)[24000:]
    frequency, spectrum = signal.periodogram(shifted, fs, window='boxcar', scaling='spectrum')
    wanted = spectrum[np.argmin(abs(frequency-2500))]
    unwanted = spectrum[np.argmin(abs(frequency-1500))]
    rejection = float(10*np.log10(unwanted/wanted))
    checks.append(dict(name='Hilbert unwanted sideband', observed_db=rejection, maximum_db=-35, passed=rejection < -35))
    assert rejection < -35, rejection
    for semitones, expected_frequency in ((12, 4000), (-12, 1000)):
        actual = render('pitch', tone, pitch=semitones)[24000:]
        frequency, spectrum = signal.periodogram(actual, fs)
        observed = float(frequency[np.argmax(spectrum)])
        verify(f'Pitch ratio {semitones:+} semitones', np.array([observed]), np.array([expected_frequency]), 2)

report = dict(python=platform.python_version(), numpy=np.__version__, scipy=scipy.__version__,
              seed=74031, sample_rate=fs, total=len(checks), passed=sum(c['passed'] for c in checks), checks=checks)
if args.output:
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2)+'\n')
print(json.dumps({'checks':report['total'], 'passed':report['passed'], 'status':'passed'}))
