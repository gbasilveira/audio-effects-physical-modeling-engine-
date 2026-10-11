# Implementation plan and decisions

> Generated view of [research-and-plans.md](research-and-plans.md), the canonical source. Edit the report and run `python docs/plan/verification/build_views.py`. R/S IDs refer to the complete report’s bibliography.

<a id="1-decisions"></a>

## 1. Decisions and relationship to the reference project

### 1.1 Recommended outcome

Build **an effects graph with several numerical kernels**. Some effects are fundamentally delay lines; others are nonlinear differential circuits, wave fields, modal banks or time–frequency transformations. Unify their interfaces, units, events, measurement and validation rather than forcing every effect into one solver.

Physical models should expose interpretable parameters: resistance, capacitance, geometry, thickness, tape speed, magnetic field, impedance and absorption. Signal effects should expose their own parameters: frequency, Q, compression ratio, window, hop, semitones and resolution. A control called “material” should exist only if it actually changes a physical model or an explicitly identified empirical mapping.

**Proposed choices:** C++20 for the engine independently of plug-in format; Python/NumPy/SciPy for reference models and identification; WDF or MNA for circuits; modal states for vibrating objects; delays/FDNs for approximate propagation and efficient reverberation; convolution for measured linear systems; STFT for spectral effects; neural models as comparative alternatives or identified hybrid components.

### 1.2 Reference analysed

Repository: [gbasilveira/plucked-physical-modeling-engine](https://github.com/gbasilveira/plucked-physical-modeling-engine).

Inspected snapshot: `49b00478e0fb5f90aea2411471d72f2ef69cbed7`. The inspected tree contains planning documentation; its README states that implementation has not begun. The review covered the README, planning index, literature survey, implementation plan and structural sections of the engine specification and phasing plan.

| Reference element | Equivalent in this report |
| --- | --- |
| `literature-survey.md` | Sections 3 and 10: literature, software, usefulness and limitations |
| `engine-specification.md` | Sections 4–6: equations, discretisation, execution and contracts |
| `implementation-plan.md` | Sections 1, 6 and 9: architecture, costs and decisions |
| `phasing-plan.md` | Section 7: phases with inputs, outputs and gates |
| `verification/` | Sections 8 and 11: independent references and reproducible experiments |
| `phases.yaml` | Machine-readable dependency graph and standard model card |

This work does not modify the reference repository. It preserves its useful separation: the specification governs behaviour, while phases govern deliverables and tests. Piano-specific choices do not automatically transfer to effects. In particular, adding a sub-block of latency inside analog feedback changes the circuit, even if it simplifies parallel execution.

### 1.3 Levels of claim

- **Physics:** a conservation, constitutive or propagation law, with units and assumptions.
- **Reduction:** a declared approximation of a more complex system, such as truncated modes or a variable-resistor JFET approximation.
- **Signal DSP:** a useful transformation without a claim to represent a particular physical object.
- **Proposal:** architecture, exercises and thresholds recommended here; these are not published results or measured performance of the future engine.

The baseline models below are implementable. Emulating a particular device also requires its circuit or mechanical design, parameter values and measurements. The plans identify that work explicitly: a generic valve equation is not a verified emulation of every amplifier.

<a id="2-coverage"></a>

## 2. Coverage and taxonomy

“All” is interpreted as coverage of the principal audio transformation families and their compositions. There is no finite list of every possible effect: new effects arise through composition, control and feedback. This table makes the scope auditable.

| ID | Family and included variants | Main nature | Kernels |
| --- | --- | --- | --- |
| FX01 | Delay, echo, slapback, multitap, ping-pong, dub | Signal / propagation | Delay, feedback matrix |
| FX02 | Tape delay, tape saturation, wow/flutter, multihead | Physics + reduction | Transport, hysteresis, filters |
| FX03 | BBD delay and analog ensemble | Sampled circuit | Clock, sample-and-hold, filters |
| FX04 | Flanger, through-zero, positive/negative feedback | Interference | Variable delay |
| FX05 | Chorus, doubler, ensemble, vibrato, ADT | Signal / transport | Multiple delays, modulation |
| FX06 | Phaser, allpass, JFET/OTA phaser | Signal / circuit | Allpass, WDF/MNA |
| FX07 | Tremolo, harmonic tremolo, autopan, chopper | Modulation | Gain, LFO, crossover |
| FX08 | Ring modulation, AM, diode ring | Signal / circuit | Multiplication, nonlinearities |
| FX09 | Frequency shifter, composed barber-pole effects | Signal | Quadrature, Hilbert transform |
| FX10 | Parametric/graphic EQ, shelves, tilt, notch, FIR, tone stack | Signal / circuit | Biquads, FIR, MNA |
| FX11 | Wah, autowah, envelope filter, ladder, SVF, resonant LP/HP/BP | Circuit / control | TPT, WDF, nonlinear solver |
| FX12 | Compressor, limiter, expander, gate, ducking, multiband | Signal / control | Detector, gain computer, lookahead |
| FX13 | Optical compression and vactrol | Physics + identification | Photoconductivity, slow states |
| FX14 | FET, VCA, vari-mu and feedback compression | Circuit | JFET, valve, sidechain |
| FX15 | Transient shaper, de-esser, dynamic EQ | Adaptive signal processing | Envelopes and bands |
| FX16 | Overdrive, distortion, fuzz, clipper, rectifier, wavefolder, exciter | Circuit / signal | WDF/MNA, ADAA |
| FX17 | Amplifier, preamp, power amp, transformer, sag | Circuit physics | DAE, magnetism, power supply |
| FX18 | Speaker, cabinet, microphone and pickup transfer | Electroacoustics / measurement | ODE, IR, modes |
| FX19 | Algorithmic reverb: Schroeder, Dattorro, FDN, shimmer, freeze | Signal / approximation | Delay networks |
| FX20 | Convolution reverb and filtering, IR morphing | Measured linear system | Partitioned FIR |
| FX21 | Room, hall, chamber, early reflections, room modes | Acoustics | Image-source, SDN, FDTD/FEM |
| FX22 | Plate reverb and two-dimensional objects | Mechanics | Plate PDE, modes |
| FX23 | Spring reverb and helical dispersion | Mechanics | Helical, allpass/modal models |
| FX24 | Spatialisation, binaural, Ambisonics, rotary/Leslie, Doppler, widening | Acoustics / signal | Geometry, HRTF, delays |
| FX25 | Pitch shifter, harmoniser, octave, tuning correction, time stretch, formants | Signal | Delays, STFT, PSOLA/WSOLA |
| FX26 | Granular, reverse, stutter, buffer freeze, glitch | Segmented signal | Buffers and windows |
| FX27 | Vocoder, talkbox, formant filters, cross-synthesis | Source–filter / acoustics | Filterbanks, LPC, tubes |
| FX28 | Spectral delay, spectral freeze, morph, blur, spectral gate | Time–frequency | STFT, per-bin filters |
| FX29 | Bitcrusher, sample-rate reduction, lo-fi, vinyl/radio/telephone | Signal / degradation | Quantisation, resampling, noise |
| FX30 | Resonators and objects excited by audio | Mechanics / acoustics | Modes, waveguides, coupling |
| FX31 | Denoise, de-click, de-hum, dereverb and inverse filters | Estimation / restoration | Spectral statistics, regularisation |
| FX32 | Multi-FX, parallel and feedback graphs, hybrid convolution, neural assistance | Composition | Graph, stability, identification |

Analysis tools, meters and tuning measurement are infrastructure, not a 33rd effects family. Semantic source separation, music generation and complete codecs are adjacent areas, not specified as engines in this version. Codec degradation can become an FX29 node, but requires a codec-specific specification.

<a id="9-risks"></a>

## 9. Engineering risks and decisions

| Risk | Consequence | Concrete action |
| --- | --- | --- |
| Confusing DSP and physics | Misleading controls and expectations | Label every model and reduction |
| Weak identifiability | Many parameters explain one curve | Independent excitation, physical priors, sensitivity and uncertainty |
| Ignoring port loading | Incorrect cascaded circuits | Solve jointly or justify impedance isolation |
| Unbounded Newton iterations | CPU spikes and missed callbacks | Operating domain, damping, warm start, iteration cap and fallback |
| Feedback with variable delay | Unexpected pitch and energy | LTV analysis and slew limits; frozen poles are insufficient |
| Modal truncation | Sample-rate-dependent timbre/energy | Explicit cutoff, weights and convergence |
| Treating IR as nonlinear | Level-dependent mismatch | Multiple measurement levels; ODE/Volterra/hybrid where needed |
| Opaque neural models | Out-of-training failure and rate dependence | Physical baseline and held-out state/control/rate tests |
| Excessive scope | No reviewable milestones | Concrete variants, gates and complete vertical slices |
| Licence mismatch | Reuse incompatible with intended distribution | Per-file/dependency/IR/dataset/weights inventory before integration |

The destination repository uses MIT. Original documentation and verification code follow that licence; cited papers/software retain their own terms. Citing GPL code does not relicense it as MIT. Studying an idea, independently implementing equations and copying code are different activities. Distribution must respect the actual versions incorporated.

**Defer until measured:** mode/FDN-line counts, interpolation order, per-effect oversampling, WDF versus MNA, partition boundaries and identification methods. **Fix now:** units, memory contracts, causality, traceability, state format and failure policy.

This delivery supplies a mathematical and testable route to implementation. It does not claim to implement every plug-in, provide uncollected hardware measurements or establish perceptual equivalence without experiments.
