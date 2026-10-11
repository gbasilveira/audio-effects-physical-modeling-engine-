# v0.1 implementation report

11 October 2026 · C++20 · First implementation milestone

The repository now contains a working mono/stereo audio engine, an offline renderer and executable validation. Every one of the research report's 32 families has a compiled baseline. Physical circuit, electromechanical and modal models coexist with ordinary DSP and explicitly reduced device models. This milestone establishes executable models and shared infrastructure; completion of a family baseline does not complete every physical extension or product acceptance gate in the research plan.

## Build and use

No third-party C++ dependency is required. Use GCC 13 or another compiler supporting C++20. CMake 3.20 or later provides library installation and an exported package.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
build/audiofx-render --list
build/audiofx-render --family diode --drive 4 --oversampling 4 --mix 1 \
  --signal music --seconds 4 --output diode.wav
build/audiofx-render --family plate --mix 1 --signal impulse \
  --seconds 5 --output plate.wav
```

On Windows, executables from a multi-configuration generator are under `build/Release`. Alternatively, `make test` builds the renderer/core tests and runs the independent Python comparisons. Install their dependencies with `python -m pip install -r docs/plan/verification/requirements.txt`.

The renderer writes IEEE float32 WAV, or CSV to stdout with `--raw`. It accepts mono/stereo CSV input with one frame per row, whitespace-separated FIR coefficients with `--ir`, and explicit silent tails with `--tail-samples`. `--help` lists options. Family names and numbers 1–32 are equivalent. Unsupported family controls are rejected. Synthetic `music` is a deterministic multitone test signal, not a listening study. Renderer smoothing is disabled for reproducible parameter experiments; the API default is 10 ms.

## API and processing contract

[`engine.hpp`](../../include/audiofx/engine.hpp) exposes `Design`, `ProcessSpec`, `Node`, `Graph`, `Event`, range metadata and diagnostics. [`dsp.hpp`](../../include/audiofx/dsp.hpp) contains the reusable numerical kernels; [`engine.cpp`](../../src/engine.cpp) assembles the models.

```cpp
#include <audiofx/engine.hpp>
#include <array>
int main() {
    audiofx::Design eq;
    eq.family = audiofx::Family::Eq;
    eq.parameters[audiofx::Parameter::Mix] = 1;
    eq.smoothingSeconds = 0;
    audiofx::Graph graph;
    graph.prepare({{eq}, {}, 0}, {48000, 128, 1});
    std::array<double, 128> input{}, output{};
    const double* in[] = {input.data()};
    double* out[] = {output.data()};
    const audiofx::Event event{64, 0, audiofx::Parameter::GainDb, 6};
    return graph.process(in, out, 128, std::span(&event, 1)) ? 0 : 1;
}
```

Prepare allocates and computes coefficients outside audio processing, validates designs and preserves the previous graph if preparation fails. Processing is `noexcept`, preallocated and bounded by the prepared graph, modal count and maximum block. Tests instrument C++ allocations during callbacks. No locks, file access or dynamic graph editing occur there. This establishes measured behavior for the tested paths, not a universal hard real-time scheduling guarantee.

- Supported rate: 8–192 kHz; channels: 1 or 2; maximum block: 1–8192 frames. Buffers are planar double precision; corresponding input/output channels may alias. Zero-frame calls are valid without audio pointers.
- Events use a block-relative sample offset and execute before that sample. They must be sorted; equal offsets execute in supplied order. Invalid structural events reject the block before writing audio. Invalid control values leave the current target unchanged and increment diagnostics. Query `supportsParameter` and `parameterRange` rather than assuming all 17 controls affect every family.
- Continuous controls use one-pole target smoothing; bits/freeze change discretely. Physical dimensions, modal decay, FIR impulse response and oversampling are design-time values. Reprepare to change them. Frequency-dependent coefficients are limited below Nyquist.
- Each graph root receives external input. Routes sum linearly; feedforward paths are automatically aligned at each merge. Cycles require an explicitly delayed edge, storing one previous audio sample. This models digital feedback, not an implicit analog circuit. Large loop gains can still become unstable: graph topology validation is not a passivity proof.
- One thread owns an active graph. Do not call prepare/reset/set/diagnostics concurrently with processing. Prepare a replacement outside the callback and perform a host-controlled safe handoff if live replacement is needed. A lock-free snapshot management layer is not included.
- Nonfinite input becomes zero; finite overload above magnitude 32 is clipped and diagnosed. Extremely small values are flushed explicitly. Nonlinear solver failures are counted. Normal-domain tests require zero failures; no hardware-equivalence claim follows from this.
- `reset` clears phase and DSP history, restores the seed and adopts the latest parameter targets without a smoothing ramp. Use explicit silence to render tails. Infinite `tailSeconds` means no finite bound is declared: recursive support, freeze or a conservative descriptor. It is not an instruction to allocate infinite output.

## Implemented coverage and remaining variants

`Physical` below means coefficients follow a stated physical circuit, mechanical or acoustic model. It does not mean parameters were identified from a commercial device. All other rows are DSP or reduced models. The canonical [32 family cards](../plan/research-and-plans.md) retain the deeper derivations, source reading and planned extensions.

| FX | Family / CLI name | Executable model | Remaining scope |
|---|---|---|---|
| 01 | `delay` | Causal ring buffer, linear fractional interpolation and feedback | Higher-order/Farrow interpolation and variable-speed comparison |
| 02 | `tape` | Play-operator hysteresis, RC head response, wow and feedback | Identified magnetic hysteresis, bias, transport and head geometry |
| 03 | `bbd` | Clocked 256-stage charge queue and input/output filtering | Analog clock feedthrough, leakage and full multirate switched circuit |
| 04 | `flanger` | Through-zero delay relative to a causal 20 ms reference | Device-specific transfer functions and higher-order interpolation |
| 05 | `chorus` | Three phase-offset fractional-delay voices | Detailed ensemble/BBD variants |
| 06 | `phaser` | Four modulated first-order allpass sections with feedback | OTA/FET nonlinear stages and component identification |
| 07 | `tremolo` | Sinusoidal gain, bounded depth | Bias modulation and optical device models |
| 08 | `ring` | Ideal balanced multiplication | Diode-ring circuit and imbalance |
| 09 | `frequency-shift` | FIR Hilbert analytic signal and positive SSB shift | Broadband image rejection and quadrature analog variants |
| 10 | `eq` | RBJ peaking biquad, DF2T | Shelves, graphic EQ, measured passive networks |
| 11 | `wah` | TPT state-variable bandpass with frequency/Q controls | Pedal mechanics and physical inductor circuit |
| 12 | `compressor` | Feedforward power envelope, RMS, hard-knee static ratio | Soft knee, lookahead and detector variants |
| 13 | `optical` | Two-state phenomenological gain recovery | Identified light-source/LDR laws |
| 14 | `vca` | Exponential gain with explicitly delayed output feedback | Implicit analog VCA/FET solution and identified devices |
| 15 | `transient` | Fast/slow envelopes and bounded exponential gain | Multiband and perceptual calibration |
| 16 | `diode` | **Physical:** RC circuit with antiparallel Shockley diodes, backward Euler/Newton | General MNA/WDF circuits, adaptive solver fallback and device measurements |
| 17 | `amplifier` | Centered triode-law transfer, RC supply sag, ADAA tanh and filtering | Full tube/transformer circuit and cabinet coupling |
| 18 | `speaker` | **Physical:** linear current/displacement/velocity states with exact held-input discretization | Radiation pressure, nonlinear suspension and thermal coupling |
| 19 | `fdn` | Four Householder feedback delay lines with specified decay | Frequency-dependent damping, higher-order FDN and perceptual tuning |
| 20 | `convolution` | Arbitrary FIR, uniform 128-sample partitioned FFT convolution | Nonuniform partitions, IR crossfades and long-IR scheduling |
| 21 | `room` | **Physical/reduced:** shoebox direct path, six first-order reflections and low room modes | Higher-order reflections, SDN, boundary-dependent damping |
| 22 | `plate` | **Physical:** simply supported rectangular thin-plate modes, force/mass coupling | Free edges, measured pickup response and nonlinear/high-order variants |
| 23 | `spring` | Dispersive allpass waveguide with loss and feedback | Helical multi-wave PDE, transducer mechanics and identification |
| 24 | `spatial` | Moving-source distance, directivity and stereo arrival differences | HRTF, binaural convolution, ambisonics and Doppler validation |
| 25 | `pitch` | Up to 16 tracked spectral partials and additive resynthesis | Full phase-vocoder, transient preservation and independent time stretch |
| 26 | `granular` | Four bounded overlapping grains with seeded delay selection | Rich grain scheduling and independent time stretch |
| 27 | `vocoder` | Eight analysis/envelope/synthesis bands with internal harmonic carrier | External carrier and LPC variants |
| 28 | `spectral` | 256-point streaming STFT with identity and phase-continuing freeze | General spectral operators and transient handling |
| 29 | `crusher` | Rounded bounded quantization and clocked sample hold | Hardware noise, reconstruction and identified ADC models |
| 30 | `resonator` | **Physical:** stiff-string modal bank, tension and bending terms | Coupled surfaces, waveguides and nonlinear excitation |
| 31 | `restoration` | Fixed-noise-power STFT subtraction with nonzero spectral floor | Learned noise estimates, Wiener filtering and dereverberation |
| 32 | `multi-fx` | Delay → EQ → diode; general graph supports parallel composition | Host integration, preset migration and full product acceptance |

## Mathematical implementation choices

The kernels are separately usable, making it possible to study the equations before composing effects. The reference equations remain in [the mathematical specification](../plan/engine-specification.md); these implementation details resolve the first milestone's numerical choices.

**Transport.** A read-before-write delay with fractional distance \(D=k+\alpha\) reads \((1-\alpha)x[n-k]+\alpha x[n-k-1]\). Minimum distance is one sample. Feedback uses the previously available delayed sample. The BBD queue transfers at twice its stage clock; discretized clock events are placed at sample boundaries. For 256 stages, 10 ms and 48 kHz, its observed impulse arrival is sample 481 under this convention. It is an event-clock approximation, not an analog switched-capacitor simulator.

**Implicit diode.** Solve \(C(v_n-v_{n-1})/T+(v_n-u_n)/R+2I_s\sinh(v_n/V_T)=0\), using \(R=2200\,\Omega\), \(C=10\,\mathrm{nF}\), \(I_s=1\,\mathrm{nA}\), \(V_T=26\,\mathrm{mV}\). The derivative is strictly positive in the supported domain. A bracketed, safeguarded Newton method has a maximum of 32 iterations, with a bounded result and diagnostic failure counter. This is one physical circuit, not a general nonlinear circuit solver.

**Oversampling and ADAA.** Diode/amplifier support 1, 2, 4 and 8 times processing rate. For factor \(K>1\), interpolate zero-stuffed input using a normalized \(16K+1\)-tap Blackman-windowed sinc with cutoff \(0.45/K\), evaluate the nonlinear model at \(Kf_s\), then filter and decimate. The two linear-phase FIRs contribute 16 base-rate samples of latency. Internal delay compensation aligns the dry signal. The amplifier's tanh stage also uses the stable log-cosh antiderivative and a small-difference limit. These measures reduce aliasing; the checked tests verify the implemented paths, not a universal alias-rejection bound.

**Physical states.** Speaker states satisfy \(L\dot i=u-Ri-B\ell v\), \(\dot q=v\), \(M\dot v=B\ell i-R_m v-Kq\). Preparation evaluates the augmented matrix exponential for exact zero-order-hold input. Output is velocity scaled to audio, not acoustic pressure. Every modal oscillator similarly uses an exact 2×2 matrix exponential and held-force input integral. Plate frequencies follow bending/tension dispersion, simply supported sine shapes and modal mass \(\rho hL_xL_y/4\). String frequencies include tension and bending stiffness. Modes above 0.45 of sample rate are omitted. Normalized input/output positions select modal coupling; `observationScale` converts displacement metres to normalized audio units. Linear superposition and constant damping are explicit assumptions.

**Reverberation.** The FDN's Householder matrix is orthogonal. Line attenuation follows \(g_i=10^{-3D_i/(f_sT_{60})}\). A dedicated lossless test checks stored energy. The room's low modes and first-order image arrivals provide an interpretable hybrid but do not solve a complete arbitrary boundary-value problem. The spring model deliberately reduces dispersion to allpass sections.

**FFT paths.** The FFT plan precomputes bit reversal and twiddles. Convolution accumulates frequency-domain products of 128-sample blocks in 256-point transforms, then overlap-adds; algorithmic latency is exactly 128 samples independent of IR length. STFT uses 256 samples, hop 64, Hann analysis/synthesis and overlap normalization 1.5, with 256-sample identity latency. Freeze retains magnitude and per-bin phase increment. Restoration estimates no changing noise statistics.

**Pitch.** This milestone uses a 1024-sample Hann window/hop 256 and at most 16 local spectral peaks. Initial peak frequency uses log-magnitude quadratic interpolation; later frames estimate instantaneous frequency from the wrapped phase deviation from the expected hop advance. Tracked oscillators advance at \(2^{s/12}\) times the estimated frequency, with smoothed amplitudes and continuous phase. At zero semitones the path returns an exact 1024-sample delayed input. Nonzero shifts are a lossy partial model: noise, dense polyphony and transients need further work. Two independent frequency tests cover an octave up/down on a 2 kHz tone; they do not establish general music quality or independent duration control.

## Validation evidence

All commands below completed locally on Linux x86-64 with GCC 13.3.0, Python 3.12.14, NumPy 2.3.5 and SciPy 1.17.0. GitHub CI additionally defines a Windows core build and Linux numerical/sanitizer jobs; remote CI results are distinct from these local results.

| Check | Evidence | Scope |
|---|---|---|
| C++ core | [core results](core-results.json), `audiofx-tests`, CTest | 32-family finite output, deterministic reset, block partition equivalence, rates 8/44.1/96/192 kHz, stereo, graph/event errors, exact event offset, delayed feedback, parallel latency; oversampling paths 2/4/8 |
| Callback allocations | Instrumented global allocation operators | Zero observed for all tested callbacks; no lock use in implementation |
| Independent oracles | [30 results](oracle-results.json), `tests/oracles.py` | NumPy/SciPy filters, convolution at partition boundaries, STFT identity, Brent diode roots including oversampling, matrix-exponential speaker, analytic plate, static compression, SSB rejection, octave pitch |
| Actual renderer files | [32 WAV results](render-results.json), `tests/render_smoke.py` | Two seconds per family, stereo float32 WAV, correct dimensions/rate, nonzero finite signal, no solver failures |
| Memory/undefined behavior | `make sanitize` | AddressSanitizer and UndefinedBehaviorSanitizer pass. Leak detection is disabled because this runtime blocks the required process inspection; no leak-clean claim is made |
| CMake package | Install + separate `find_package(audiofx CONFIG REQUIRED)` consumer | Exported target, headers, compile/link/run |
| Planning consistency | `check_plan.py` and generated views | Family IDs, phase dependencies, references and local links |

The oracle suite independently evaluates equations using SciPy filters, bracketed root finding, matrix exponentials and direct convolution. It avoids a second literal translation of the C++ implementation wherever practical. Its results apply to the listed signals/parameters; physical identification, blind listening, exhaustive extremes, every family automation regime, arbitrary host load and maximum-size graph benchmarks remain uncompleted.

[`benchmark-results.json`](benchmark-results.json) reports measured median, p99 and maximum callback time for 1024 warmed 128-frame stereo blocks per family at 48 kHz, with default designs. `median_realtime_fraction` divides the median callback duration by the block duration. These measurements share a container and are descriptive, not a deadline guarantee, dedicated hardware benchmark or comparison to commercial plug-ins. Long IRs and maximum modal/graph sizes need separate budgets. Run `build/audiofx-benchmark` to reproduce on the target computer.

## Next implementation gates

The [phase YAML](../plan/phases.yaml) marks the core/conventions complete for this milestone and the remaining phases in progress. Finish the remaining six-artifact model contracts rather than treating this baseline matrix as product completion. Priority: general circuit solvers and device identification; transport/antialiasing error studies; spring and room physical extensions; complete pitch/time algorithms; then plug-in SDK wrappers, host automation/safe graph replacement, measured presets and listening validation. Source papers and software retain their own licenses; the implementation here is original and uses the repository's MIT license.
