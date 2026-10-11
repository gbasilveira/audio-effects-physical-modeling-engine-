#!/usr/bin/env python3
"""Independent numerical checks of selected report equations, not an FX engine.

Run from anywhere. Outputs are written next to this script and in ../figures.
All signals are synthetic; float64 throughout; no third-party DSP code copied.
"""
from pathlib import Path
import json
import platform
import numpy as np
import scipy
from scipy import signal, linalg, optimize, integrate
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

HERE = Path(__file__).resolve().parent
RNG = np.random.default_rng(74031)
RESULTS = []


def check(name, metrics, passed, scope):
    RESULTS.append(dict(name=name, passed=bool(passed), metrics=metrics, scope=scope))
    if not passed:
        raise AssertionError(f'{name}: {metrics}')


def rbj(fs, fc, q, gain_db):
    A = 10 ** (gain_db / 40)
    w = 2*np.pi*fc/fs
    alpha = np.sin(w)/(2*q)
    b = np.array([1+alpha*A, -2*np.cos(w), 1-alpha*A])
    a = np.array([1+alpha/A, -2*np.cos(w), 1-alpha/A])
    return b/a[0], a/a[0]


def main():
    fs = 48000
    # 1: digital EQ has expected center gain and unity DC/Nyquist.
    eq_errors = []
    for sr in (44100, 48000, 96000):
        for fc in (100, 1000, 10000):
            for q in (.5, 1, 8):
                for db in (-12, 0, 12):
                    b, a = rbj(sr, fc, q, db)
                    _, H = signal.freqz(b, a, worN=[0, 2*np.pi*fc/sr, np.pi])
                    eq_errors.append(float(np.max(np.abs(20*np.log10(abs(H))-[0, db, 0]))))
    check('RBJ center and endpoints', {'max_error_db': max(eq_errors), 'cases': len(eq_errors)},
          max(eq_errors) < 1e-8, '81 static designs, three sample rates; no automation test')

    # 2: TPT SVF recurrence versus an independently transformed analog transfer.
    svf_errors = []
    for fc in (80, 1000, 9000):
        for q in (.5, 1, 6):
            g, k = np.tan(np.pi*fc/fs), 1/q
            omega = 2*fs*g
            b, a = signal.bilinear([omega**2], [1, k*omega, omega**2], fs)
            x = RNG.normal(size=4096)
            expected = signal.lfilter(b, a, x)
            s1 = s2 = 0.
            actual = []
            for u in x:
                v1 = (s1+g*(u-s2))/(1+g*k+g*g)
                v2 = s2+g*v1
                s1, s2 = 2*v1-s1, 2*v2-s2
                actual.append(v2)
            svf_errors.append(float(np.linalg.norm(actual-expected)/np.linalg.norm(expected)))
    check('TPT SVF versus bilinear', {'max_relative_l2': max(svf_errors), 'cases': 9},
          max(svf_errors) < 1e-9, 'LP output, fixed coefficients, independent transfer-function oracle')

    # 3: capacitor WDF boundary relation, source resistor R, ideal voltage u.
    R, C = 2200., 100e-9
    Rp = 1/(2*fs*C)
    reflection = 0.
    x = RNG.normal(size=2048)
    actual = []
    for u in x:
        # (a+b)/2 = u - R*(a-b)/(2*Rp)
        incident = (2*Rp*u+(R-Rp)*reflection)/(R+Rp)
        actual.append((incident+reflection)/2)
        reflection = incident
    b, a = signal.bilinear([1], [R*C, 1], fs)
    err = float(np.max(np.abs(np.array(actual)-signal.lfilter(b, a, x))))
    check('WDF RC versus bilinear', {'max_abs_volt': err}, err < 1e-12,
          'Linear capacitor reflection sign and port resistance; not arbitrary WDF topology')

    # 4: actual integer feedback delay impulse, not just evaluating the gain formula.
    m, t60 = 4800, 1.0
    gain = 10**(-3*m/(fs*t60))
    delay = np.zeros(m)
    echoes = []
    for n in range(fs+1):
        i = n % m
        delayed = delay[i]
        value = (1. if n == 0 else 0.) + gain*delayed
        delay[i] = value
        if n % m == 0:
            echoes.append(value)
    drop = float(20*np.log10(abs(echoes[-1]/echoes[0])))
    check('Feedback delay T60', {'drop_at_one_second_db': drop, 'loop_gain': gain},
          abs(drop+60) < 1e-10, 'Fixed integer delay 0.1 s, broadband scalar damping')

    # 5: FIR flanger cancellation evaluated via polynomial frequency response.
    b = np.zeros(49)
    b[0] = b[48] = .5
    freqs = (2*np.arange(24)+1)*fs/(2*48)
    _, H = signal.freqz(b, worN=2*np.pi*freqs/fs)
    null = float(np.max(np.abs(H)))
    check('Frozen flanger nulls', {'max_magnitude_at_nulls': null}, null < 1e-12,
          'Equal dry/wet, positive feedforward, integer delay 48 samples; no modulation')

    # 6: full buffer energy of lossless and damped FDN.
    A = np.eye(4)-2*np.ones((4, 4))/4
    lengths = [37, 53, 71, 89]
    initial = [RNG.normal(size=n) for n in lengths]
    def fdn(gains):
        buffers = [v.copy() for v in initial]
        E = [sum(float(v@v) for v in buffers)]
        for n in range(4096):
            out = np.array([v[n % len(v)] for v in buffers])
            incoming = A@(gains*out)
            for j, v in enumerate(buffers):
                v[n % len(v)] = incoming[j]
            E.append(sum(float(v@v) for v in buffers))
        return np.asarray(E)/E[0]
    lossless = fdn(np.ones(4))
    damped = fdn(10**(-3*np.array(lengths)/(fs*.15)))
    drift = float(np.max(np.abs(lossless-1)))
    increase = float(np.max(np.diff(damped)))
    check('FDN buffer energy', {'lossless_relative_drift': drift, 'max_damped_energy_step': increase},
          drift < 1e-12 and increase < 1e-12,
          'Four fixed delay lines and orthogonal mixing; does not cover modulated or nonlinear feedback')

    # 7: modal matrix exponential versus closed-form homogeneous motion.
    omega, sigma = 2*np.pi*730, 6.9
    wd = np.sqrt(omega**2-sigma**2)
    M = np.array([[0., 1.], [-omega**2, -2*sigma]])
    step = linalg.expm(M/fs)
    state = np.array([1., 0.])
    out, energy = [], []
    for _ in range(4096):
        out.append(state[0])
        energy.append(.5*(state[1]**2+omega**2*state[0]**2))
        state = step@state
    t = np.arange(len(out))/fs
    exact = np.exp(-sigma*t)*(np.cos(wd*t)+sigma/wd*np.sin(wd*t))
    err = float(np.max(np.abs(out-exact)))
    increase = float(np.max(np.diff(energy))/energy[0])
    check('Modal exact step and energy', {'max_abs_displacement': err, 'max_relative_energy_step': increase},
          err < 1e-10 and increase <= 1e-12, 'Unforced damped oscillator, positive mass and damping')

    # 8: modal projection mass by quadrature and physical plate scaling.
    rho, h, lx, ly, E, nu = 7800., .0005, 2., 1., 2e11, .3
    integral, _ = integrate.dblquad(lambda y, x: rho*h*(np.sin(3*np.pi*x/lx)*np.sin(2*np.pi*y/ly))**2,
                                   0, lx, lambda x: 0, lambda x: ly, epsabs=1e-11)
    mass = rho*h*lx*ly/4
    def plate_freq(thick, xsize, ysize, m=3, n=2):
        D = E*thick**3/(12*(1-nu**2))
        k2 = (m*np.pi/xsize)**2+(n*np.pi/ysize)**2
        return np.sqrt(D*k2*k2/(rho*thick))/(2*np.pi)
    f0 = plate_freq(h, lx, ly)
    ratios = [plate_freq(2*h, lx, ly)/f0, plate_freq(h, 2*lx, 2*ly)/f0]
    check('Plate mass and scaling', {'mass_quadrature_kg': integral, 'mass_formula_kg': mass,
                                   'thickness_ratio': ratios[0], 'size_ratio': ratios[1]},
          abs(integral-mass) < 1e-10 and np.allclose(ratios, [2, .25], rtol=1e-12),
          'Thin simply supported isotropic plate, zero tension; not free or clamped boundaries')

    # 9: nonlinear diode BE step with safeguarded Newton, independent bracketing.
    R, C, Is, thermal = 2200., 10e-9, 1e-9, .026
    ct = C*fs
    old = 0.
    errors, iterations, residuals = [], [], []
    x = 2*np.sin(2*np.pi*997*np.arange(4096)/fs)
    diode_y = []
    for u in x:
        def residual(v):
            return ct*(v-old)+(v-u)/R+2*Is*np.sinh(v/thermal)
        def derivative(v):
            return ct+1/R+2*Is/thermal*np.cosh(v/thermal)
        lo, hi = min(old, u, 0.)-1e-6, max(old, u, 0.)+1e-6
        reference = optimize.brentq(residual, lo, hi, xtol=1e-14)
        v = float(old)
        for it in range(64):
            r = residual(v)
            if abs(r) < 1e-13:
                break
            if r > 0:
                hi = v
            else:
                lo = v
            proposal = v-r/derivative(v)
            v = proposal if lo < proposal < hi else (lo+hi)/2
        errors.append(abs(v-reference))
        residuals.append(abs(residual(v)))
        iterations.append(it+1)
        diode_y.append(v)
        old = v
    check('Diode implicit root', {'max_newton_brent_volt': max(errors),
                                'max_residual_ampere': max(residuals), 'max_iterations': max(iterations)},
          max(errors) < 1e-8 and max(residuals) < 1e-12,
          'One synthetic signal, backward Euler scalar equation; no hardware identification or RT bound claim')

    # 10: ADAA f(x)=x must be a two-sample average, not identity.
    x = RNG.normal(size=1024)
    previous = np.r_[0., x[:-1]]
    divided = (.5*x*x-.5*previous*previous)/(x-previous)
    expected = .5*(x+previous)
    err = float(np.max(np.abs(divided-expected)))
    check('ADAA linear limit', {'max_abs_error': err}, err < 1e-10,
          'Demonstrates added averaging in first-order ADAA; does not measure nonlinear alias rejection')

    # 11: FFT overlap-add versus direct FIR, including a short last block.
    x, hfir, block = RNG.normal(size=1031), RNG.normal(size=173), 64
    nfft = 1 << (block+len(hfir)-2).bit_length()
    H = np.fft.rfft(hfir, nfft)
    y = np.zeros(len(x)+len(hfir)-1)
    for start in range(0, len(x), block):
        part = x[start:start+block]
        conv = np.fft.irfft(np.fft.rfft(part, nfft)*H, nfft)[:len(part)+len(hfir)-1]
        y[start:start+len(conv)] += conv
    expected = np.convolve(x, hfir)
    err = float(np.linalg.norm(y-expected)/np.linalg.norm(expected))
    check('FFT overlap-add convolution', {'relative_l2': err}, err < 1e-12,
          'Single FIR FFT overlap-add baseline, not a multithreaded partition scheduler')

    # 12: STFT analysis/synthesis with explicit finite-signal edge normalization.
    N, hop = 256, 64
    x = RNG.normal(size=2053)
    padded = np.pad(x, (N, N))
    window = signal.windows.hann(N, sym=False)
    y, weight = np.zeros_like(padded), np.zeros_like(padded)
    for start in range(0, len(padded)-N+1, hop):
        spectrum = np.fft.rfft(padded[start:start+N]*window)
        y[start:start+N] += np.fft.irfft(spectrum, N)*window
        weight[start:start+N] += window**2
    sl = slice(N, N+len(x))
    reconstructed = y[sl]/weight[sl]
    err = float(np.linalg.norm(reconstructed-x)/np.linalg.norm(x))
    check('STFT identity', {'relative_l2': err, 'minimum_retained_weight': float(np.min(weight[sl]))},
          err < 1e-12, 'Static Hann windows, hop 64, finite-signal padding and pointwise normalization')

    # Publication-style plots of these synthetic checks, not measured devices.
    plt.rcParams.update({'font.size': 10, 'axes.spines.top': False, 'axes.spines.right': False})
    fig, axes = plt.subplots(2, 2, figsize=(11, 7.6), constrained_layout=True)
    b, a = rbj(fs, 1000, 1, 12)
    f, H = signal.freqz(b, a, worN=8192, fs=fs)
    axes[0, 0].semilogx(f[1:], 20*np.log10(abs(H[1:])), color='#155e75')
    axes[0, 0].set(xlabel='Frequency (Hz)', ylabel='Magnitude (dB)', title='EQ: +12 dB, 1 kHz, Q = 1', xlim=(20, 20000))
    axes[0, 1].plot(np.arange(len(lossless))/fs, 10*np.log10(lossless), label='Lossless')
    axes[0, 1].plot(np.arange(len(damped))/fs, 10*np.log10(damped), label='Damped')
    axes[0, 1].set(xlabel='Time (s)', ylabel='Relative energy (dB)', title='FDN: total buffer energy')
    axes[0, 1].legend()
    axes[1, 0].plot(np.arange(180)/fs*1000, out[:180], color='#155e75')
    axes[1, 0].set(xlabel='Time (ms)', ylabel='Normalised displacement', title='Mode: 730 Hz, damping 6.9 s⁻¹')
    axes[1, 1].plot(np.arange(180)/fs*1000, 2*np.sin(2*np.pi*997*np.arange(180)/fs), label='Input')
    axes[1, 1].plot(np.arange(180)/fs*1000, diode_y[:180], label='RC + diode output')
    axes[1, 1].set(xlabel='Time (ms)', ylabel='Voltage (V)', title='Clipper: synthetic dynamic response')
    axes[1, 1].legend()
    for ax in axes.flat:
        ax.grid(alpha=.2)
    fig.suptitle('Mathematical verification • 48 kHz • float64', fontsize=15)
    out_dir = HERE.parent/'figures'
    out_dir.mkdir(exist_ok=True)
    fig.savefig(out_dir/'model-checks.png', dpi=170)
    plt.close(fig)

    report = dict(seed=74031, python=platform.python_version(), numpy=np.__version__,
                  scipy=scipy.__version__, matplotlib=matplotlib.__version__,
                  checks=RESULTS, passed=sum(r['passed'] for r in RESULTS), total=len(RESULTS))
    (HERE/'results.json').write_text(json.dumps(report, indent=2, ensure_ascii=False)+'\n')
    lines = ['| Experiment | Observed result |', '| --- | --- |']
    for r in RESULTS:
        values = '; '.join(f'{k} = {v:.6g}' if isinstance(v, (float, int)) else f'{k} = {v}'
                           for k, v in r['metrics'].items())
        lines.append(f"| {r['name']} | {values} |")
    (HERE/'results.md').write_text('\n'.join(lines)+'\n')
    print(json.dumps({'passed': report['passed'], 'total': report['total'],
                      'results': str(HERE/'results.json')}, ensure_ascii=False))


if __name__ == '__main__':
    main()
