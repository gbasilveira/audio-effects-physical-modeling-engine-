# Research and planning index

The [complete report](research-and-plans.md) is canonical. Focused views retain its section numbering:

| Document | Contents | Read this when… |
| --- | --- | --- |
| [literature-survey.md](literature-survey.md) | Sections 3 and 10 | Looking for papers, software and evidence limitations |
| [engine-specification.md](engine-specification.md) | Sections 4–6 | Studying equations, states, discretisation and contracts |
| [implementation-plan.md](implementation-plan.md) | Sections 1–2 and 9 | Deciding scope, architecture and priorities |
| [phasing-plan.md](phasing-plan.md) | Sections 7–8 and 11 | Turning study into deliverables and tests |
| [phases.yaml](phases.yaml) | 17-phase DAG and FX coverage | Tracking dependencies and progress |
| [verification/results.md](verification/results.md) | Results of 12 experiments | Checking the mathematical cases already executed |

## Three reading routes

**Understand:** foundations N01–N08 → selected FX card → references → analytic prediction → experiment → explanation of discrepancies.

**Implement:** P00–P03 → simple delay/EQ/modulation baseline → required physical-model kernel → independent reference → RT → acceptance gates. Physical phaser/ring-modulator variants additionally require P04.

**Review:** taxonomy → card assumptions → equations/discretisation → reference independence → results → limitations → licences.

## Component plans

| ID | Plan |
| --- | --- |
| FX01 | [Delay, echo, multitap and ping-pong](research-and-plans.md#fx01) |
| FX02 | [Magnetic tape, tape delay and wow/flutter](research-and-plans.md#fx02) |
| FX03 | [Bucket-brigade device: BBD](research-and-plans.md#fx03) |
| FX04 | [Flanger and through-zero flanging](research-and-plans.md#fx04) |
| FX05 | [Chorus, ensemble, doubler and vibrato](research-and-plans.md#fx05) |
| FX06 | [Phaser, allpass and JFET stages](research-and-plans.md#fx06) |
| FX07 | [Tremolo, harmonic tremolo and autopan](research-and-plans.md#fx07) |
| FX08 | [Ring modulation and AM](research-and-plans.md#fx08) |
| FX09 | [Frequency shifter](research-and-plans.md#fx09) |
| FX10 | [Equalisation, corrective filters and tone stacks](research-and-plans.md#fx10) |
| FX11 | [SVF, resonant filters, ladder and wah](research-and-plans.md#fx11) |
| FX12 | [Compressor, limiter, expander, gate and ducking](research-and-plans.md#fx12) |
| FX13 | [Optical compression and vactrols](research-and-plans.md#fx13) |
| FX14 | [FET, VCA, vari-mu and feedback compression](research-and-plans.md#fx14) |
| FX15 | [Transient shaper, de-esser and dynamic EQ](research-and-plans.md#fx15) |
| FX16 | [Distortion, overdrive, fuzz, clipping and wavefolding](research-and-plans.md#fx16) |
| FX17 | [Amplifiers, valves, transformers and sag](research-and-plans.md#fx17) |
| FX18 | [Speaker, cabinet, microphone and pickup transfer](research-and-plans.md#fx18) |
| FX19 | [Algorithmic reverberation: Schroeder, Dattorro and FDN](research-and-plans.md#fx19) |
| FX20 | [Partitioned convolution and IR morphing](research-and-plans.md#fx20) |
| FX21 | [Rooms, early reflections, room modes and SDN](research-and-plans.md#fx21) |
| FX22 | [Physical plate reverberation](research-and-plans.md#fx22) |
| FX23 | [Physical spring reverberation](research-and-plans.md#fx23) |
| FX24 | [Spatialisation, binaural, Ambisonics, Doppler and Leslie](research-and-plans.md#fx24) |
| FX25 | [Pitch shifting, harmonisers, octave and time stretching](research-and-plans.md#fx25) |
| FX26 | [Granular processing, reverse, stutter and buffer freeze](research-and-plans.md#fx26) |
| FX27 | [Vocoder, talkbox and formant modelling](research-and-plans.md#fx27) |
| FX28 | [Spectral delay, freeze, morphing and blur](research-and-plans.md#fx28) |
| FX29 | [Bitcrushing, sample-rate reduction and lo-fi](research-and-plans.md#fx29) |
| FX30 | [Resonators and objects excited by audio](research-and-plans.md#fx30) |
| FX31 | [Denoise, de-click, de-hum, dereverb and inverse filters](research-and-plans.md#fx31) |
| FX32 | [Composition, feedback and hybrid/neural models](research-and-plans.md#fx32) |
