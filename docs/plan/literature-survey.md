# Literature and software survey

> Generated view of [research-and-plans.md](research-and-plans.md), the canonical source. Edit the report and run `python docs/plan/verification/build_views.py`. R/S IDs refer to the complete report’s bibliography.

<a id="3-research"></a>

## 3. Research findings and choices

### 3.1 The most useful research strands

**DAFx and virtual analog.** The DAFx archive connects device descriptions to component models. Work by Holters, Zölzer, Yeh, Werner, Parker, Bernardini and collaborators is useful for circuits, WDFs, nonlinear reduction and analog sampling models. See [R04–R12], [R24–R28].

**Acoustics and structures.** Bilbao and Parker address spring dispersion and numerical difficulties; Bilbao, Arcas, Chaigne, Ducceschi and Webb address plates. For rooms, SDN and FDN literature distinguishes geometric approximation from perceptual quality. See [R14–R22].

**Libraries as executable documentation.** Faust provides small inspectable building blocks. `chowdsp_wdf` connects WDF formulations to C++. `fdnToolbox` supports research and should not be mistaken for a permissively licensed product dependency. `ngspice` supplies an independent circuit reference. See [S01–S06], [S12].

**Identification and differentiable models.** The model comparison in [R29] and sample-rate work in [R30] help assess where neural methods are useful. Matching a waveform does not necessarily allow resistances, geometry or materials to be changed meaningfully.

### 3.2 Reading selection by technical value

| Reading | Contribution | Practical decision |
| --- | --- | --- |
| DAFx, second edition [R01] | Broad taxonomy and connections between families | Use as a map, not a single specification |
| Smith, Physical Audio Signal Processing [R02] | Delays, scattering, waveguides and resonance | Develop intuition for propagation and feedback |
| Zavalishin [R03] | Topology-preserving VA filter design | Derive SVF/TPT before copying coefficients |
| EQ Cookbook and EQ review [R04–R05] | Biquads and alternative equalisation methods | Separate functional EQ from device-specific circuits |
| Holters–Parker BBD [R08] | Variable sampling and input/output filters | Do not reduce BBD modelling to a darkened delay |
| Chowdhury, tape [R09] | Magnetisation connected to audio processing | Treat hysteresis as state, not `tanh` |
| Phaser 2014 and 2024 [R10–R11] | Nonlinear circuit versus variable JFET reduction | Measure the reduction's valid operating region |
| ADAA 2016 and WDF/ADAA 2021 [R12–R13] | Aliasing reduction and changes to dynamics | Derive ADAA jointly with feedback |
| Lossless FDNs [R15] | Network energy structure | Use orthogonal mixing with controlled losses |
| SDN 2015 / RARN 2024 [R17–R18] | Geometry without full-field simulation cost | Starting point for editable rooms |
| Spring 2010/2013 [R21–R22] | Dispersive waves and multiple branches | Retain a physical reference when fitting reductions |
| Compressor tutorial / Fairchild / vactrol [R23–R25] | Dynamic control and physical gain elements | Maintain separate models and validation suites |

### 3.3 Recent work worth prototyping

**2024 — WDF phaser with a JFET resistance approximation [R11].** Supplementary material includes MATLAB, LTspice and a plug-in. This provides a useful comparison between an efficient reduction and circuit simulation. Small channel voltage is part of the approximation; it does not establish overload fidelity.

**2025 — ADAA through lookup-table integration [R31].** This supports curves without convenient symbolic antiderivatives. Compare table interpolation error, cancellation, memory use and aliasing against analytic alternatives.

**2025 — Black-box/gray-box comparison and ToneTwist AFx [R29].** Useful for benchmarking several device classes and studying control conditioning. Learned parameters are not automatically identifiable physical quantities.

**2025 — Stability during RNN control changes [R32].** This work examines artefacts introduced by time-varying controls. Include silence with aggressive automation in tests of neural nodes; static audio error does not cover that case.

**2026 — Parameter Estimation via Differentiable Modal Plate Synthesis [R33].** The DAFx26 challenge paper studies parameter estimation from synthetic IRs. Its evaluation on eight synthetic IRs is a useful inverse-problem experiment, not evidence of universal material recovery from real recordings. Investigate identifiability separately.

### 3.4 Evidence policy

Technical sources are papers, author publications, official documentation and original repositories. The bibliography distinguishes consulted technical text, abstract/metadata and software documentation. Third-party products were not comprehensively audited or executed. Search indexing dates were not treated as publication dates. Architecture and acceptance thresholds are proposals; original numerical experiments are explicitly identified.

<a id="10-sources"></a>

## 10. Bibliography and software

### 10.1 Search method and limitations

Family-directed research covered the DAFx archive, AES/IEEE publications, author institutional repositories, online technical books, official documentation and original code repositories. Queries included *virtual analog*, *wave digital*, *nodal DK*, *physical modelling*, *fractional delay*, *bucket brigade*, *plate/spring reverberation*, *FDN*, *differentiable audio effects*, *antiderivative antialiasing* and individual effect families.

Research date: 11 October 2026. The survey includes foundational work and verifiable 2024–2026 results. It is a directed review, not a systematic-review claim of exhaustiveness. Access labels: **T** = technical text/PDF or chapters consulted, without implying every published experiment was reproduced; **M** = primary-source metadata/abstract consulted, full reading still recommended; **D** = official documentation/README consulted; **G** = repository/metadata verified, code not yet audited or executed. Abstract-only sources do not support reproduced performance claims here.

The references form a study route. Baseline equations and implementation choices are an engineering synthesis, not copied implementations. Citation IDs connect plans to relevant reading. Reproducing a particular published device emulation still requires its full paper and supplementary material.

### 10.2 Books, papers and publications

| ID | Reference and primary link | Access and use |
| --- | --- | --- |
| R01 | U. Zölzer (ed.), *DAFX: Digital Audio Effects*, 2nd ed., Wiley, 2011. [Book page and supplementary chapters](https://www.dafx.de/DAFX_Book_Page_2nd_edition/index.html); [DOI](https://doi.org/10.1002/9781119991298) | T for online materials; broad DSP map. Access to examples does not imply access to the entire book. |
| R02 | J. O. Smith, *Physical Audio Signal Processing*. [Online book](https://www.dsprelated.com/freebooks/pasp/) | T; waveguides, delays, modulation, physical models and reverberation. Author text on a public mirror. |
| R03 | V. Zavalishin, *The Art of VA Filter Design*, revision 2.1.2, 2020. [PDF](https://www.discodsp.net/VAFilterDesign_2.1.2.pdf) | T; TPT, integration and virtual analog filters; public copy of the author’s text. |
| R04 | R. Bristow-Johnson, *Audio EQ Cookbook*, W3C, 2021. [Technical note](https://www.w3.org/TR/audio-eq-cookbook/) | T; coefficients and Q/gain definitions. Check each filter family’s conventions. |
| R05 | V. Välimäki and J. D. Reiss, *All About Audio Equalization: Solutions and Frontiers*, Applied Sciences 6(5), 129, 2016. [Author record](https://research.aalto.fi/en/publications/all-about-audio-equalization-solutions-and-frontiers/); [DOI](https://doi.org/10.3390/app6050129) | M; EQ review and entry point to advanced methods. |
| R06 | D. T. Yeh, J. S. Abel and J. O. Smith, *Simplified, Physically-Informed Models of Distortion and Overdrive Guitar Effects Pedals*, DAFx 2007. [PDF](https://dafx.de/paper-archive/2007/Papers/p189.pdf) | T; RC clipper derivation and the distinction between a dynamic circuit and reduced filter/nonlinearity cascades. |
| R07 | K. Werner, J. O. Smith and J. S. Abel, *Wave Digital Filter Adaptors for Arbitrary Topologies and Multiport Linear Elements*, DAFx 2015. [Archive](https://www.dafx.de/paper-archive/details.php?id=dFlSsTeUmyOhfdA7HIi7Aw) | M; MNA-derived adaptors extending WDF beyond series/parallel topologies. |
| R08 | M. Holters and J. Parker, *A Combined Model for a Bucket Brigade Device and its Input and Output Filters*, DAFx 2018. [Author PDF](https://www.hsu-hh.de/ant/wp-content/uploads/sites/699/2018/09/Holters-Parker-2018-A-Combined-Model-for-a-Bucket-Brigade-Device-and-its-Input-and-Output-Filters.pdf) | T; clock events and interaction with input/output filters. |
| R09 | J. Chowdhury, *Real-time Physical Modelling for Analog Tape Machines*, DAFx 2019. [PDF](https://www.dafx.de/paper-archive/2019/DAFx2019_paper_3.pdf) | T; transport, magnetisation and losses. Identify parameters before generalising to other tape machines. |
| R10 | F. Eichas, M. Fink, M. Holters and U. Zölzer, *Physical Modeling of the MXR Phase 90 Guitar Effect Pedal*, DAFx 2014. [PDF](https://dafx.de/paper-archive/2014/dafx14_felix_eichas_physical_modeling_of_the_.pdf) | M; comparison with a more complete phaser circuit approach. |
| R11 | R. Giampiccolo et al., *Wave Digital Model of the MXR Phase 90 Based on a Time-Varying Resistor Approximation of JFET Elements*, DAFx 2024. [PDF](https://www.dafx.de/paper-archive/2024/papers/DAFx24_paper_13.pdf) | T; interpretable JFET reduction, valid within its resistive-approximation operating domain. |
| R12 | J. Parker, V. Zavalishin and E. Le Bivic, *Reducing the Aliasing of Nonlinear Waveshaping Using Continuous-Time Convolution*, DAFx 2016. [PDF](https://www.dafx.de/paper-archive/2016/dafxpapers/20-DAFx-16_paper_41-PN.pdf) | T; foundations of antiderivative antialiasing for nonlinear waveshaping. |
| R13 | D. Albertini, A. Bernardini and A. Sarti, *Antiderivative Antialiasing Techniques in Nonlinear Wave Digital Structures*, 2021. [Institutional manuscript](https://re.public.polimi.it/retrieve/e0c31c12-807b-4599-e053-1705fe0aef77/21115.pdf) | T; ADAA in WDF structures; feedback incorporation requires dynamic analysis. |
| R14 | V. Välimäki, J. Parker, L. Savioja, J. O. Smith and J. Abel, *Fifty Years of Artificial Reverberation*, IEEE TASLP 20(5), 1421–1448, 2012. [Record](https://research.aalto.fi/en/publications/fifty-years-of-artificial-reverberation/); [DOI](https://doi.org/10.1109/TASL.2012.2189567) | M; review organising Schroeder structures, networks, acoustics and approximations. |
| R15 | S. J. Schlecht and E. A. P. Habets, *On Lossless Feedback Delay Networks*, IEEE TSP, 2017. [Preprint](https://arxiv.org/abs/1606.07729); [DOI](https://doi.org/10.1109/TSP.2016.2637323) | M; losslessness conditions. Read the full theorem for cases beyond the orthogonal construction derived here. |
| R16 | S. J. Schlecht, *FDNTB: The Feedback Delay Network Toolbox*, DAFx 2020. [Author page and paper](https://www.sebastianjiroschlecht.com/project/fdntb/) | D/M; matrix analysis, poles and MATLAB FDN prototypes. |
| R17 | E. De Sena, H. Hacıhabiboğlu, Z. Cvetković and J. O. Smith, *Efficient Synthesis of Room Acoustics via Scattering Delay Networks*, IEEE TASLP, 2015. [Preprint](https://arxiv.org/abs/1502.05751) | M; SDN and the distinction between first-reflection accuracy and higher-order approximation. |
| R18 | S. Scerbo, L. Savioja and E. De Sena, *Room Acoustic Rendering Networks with Control of Scattering and Early Reflections*, IEEE TASLP 32, 3745–3758, 2024. [Record](https://research.aalto.fi/en/publications/room-acoustic-rendering-networks-with-control-of-scattering-and-e/); [DOI](https://doi.org/10.1109/TASLP.2024.3436702) | M; acoustic networks with control of scattering and early reflections. |
| R19 | S. Bilbao, G. Arcas and A. Chaigne, *A Physical Model for Plate Reverberation*, ICASSP 2006. [DOI](https://doi.org/10.1109/ICASSP.2006.1661238) | M; physical plate reference. The analytic baseline here does not reproduce the complete published solver. |
| R20 | M. Ducceschi and C. J. Webb, *Plate Reverberation: Towards the Development of a Real-Time Physical Model for the Working Musician*, ICA 2016. [Proceedings PDF](https://www.ica2016.org.ar/ica2016proceedings/ica2016/ICA2016-0559.pdf) | M; route to a real-time physical plate. PDF access was intermittent. |
| R21 | S. Bilbao and J. Parker, *A Virtual Model of Spring Reverberation*, IEEE TASLP, 2010. [Institutional repository](https://era.ed.ac.uk/server/api/core/bitstreams/659b1704-c8f1-48ed-a881-1b6c2031c274/content) | M; physical/dispersive spring modelling. |
| R22 | S. Bilbao, *Numerical Simulation of Spring Reverberation*, DAFx 2013. [PDF](https://www.dafx.de/paper-archive/2013/papers/22.dafx2013_submission_19.pdf) | T; coupled helical motion, boundary conditions and numerical cost. |
| R23 | D. Giannoulis, M. Massberg and J. D. Reiss, *Digital Dynamic Range Compressor Design—A Tutorial and Analysis*, JAES 60(6), 399–408, 2012. [AES](https://aes2.org/publications/elibrary-page/?id=16354) | M; detector, gain and smoothing architectures; central dynamics reading. |
| R24 | P. Raffensperger, *Toward a Wave Digital Filter Model of the Fairchild 670 Limiter*, DAFx 2012. [PDF](https://www.dafx.de/paper-archive/2012/papers/dafx12_submission_9.pdf) | T; circuit and reduced blocks; title follows the PDF. |
| R25 | J. Najnudel, R. Müller, T. Hélie and D. Roze, *Power-Balanced Dynamic Modeling of Vactrols, Application to the VTL5C3/2*, DAFx 2023. [PDF](https://dafx.de/paper-archive/2023/DAFx23_paper_50.pdf) | T; physical dynamics compared with fitting attack/release constants. |
| R26 | J. Parker, *A Simple Digital Model of the Diode-Based Ring-Modulator*, DAFx 2011. [PDF](https://www.dafx.de/paper-archive/2011/Papers/66_e.pdf) | T; practical reduction of a diode ring modulator. |
| R27 | *Digital Simulation of the Diode Ring Modulator for Musical Applications*, DAFx 2008. [PDF](https://www.dafx.de/paper-archive/2008/papers/dafx08_29.pdf) | M; complementary circuit approach for FX08. |
| R28 | M. Holters and U. Zölzer, *Circuit Simulation with Inductors and Transformers Based on the Jiles–Atherton Model of Magnetization*, DAFx 2016. [PDF](https://www.dafx.de/paper-archive/2016/dafxpapers/08-DAFx-16_paper_10-PN.pdf) | T; magnetic memory and coupled circuits. |
| R29 | M. Comunità, C. J. Steinmetz and J. D. Reiss, *Differentiable Black-box and Gray-box Modeling of Nonlinear Audio Effects*, 2025. [Preprint](https://arxiv.org/abs/2502.14405) | M; differentiable structures and physical/learned model comparisons. |
| R30 | A. Carson, A. Wright, J. Chowdhury, V. Välimäki and S. Bilbao, *Sample Rate Independent Recurrent Neural Networks for Audio Effects Processing*, 2024. [Preprint](https://arxiv.org/abs/2406.06293) | M; sample-rate dependence of recurrent models. |
| R31 | L. Gabrielli and S. Squartini, *Simplifying Antiderivative Antialiasing with Lookup Table Integration*, DAFx 2025. [PDF](https://www.dafx.de/paper-archive/2025/DAFx25_paper_30.pdf) | M; LUT-based ADAA implementation option; measure error and memory in the prototype. |
| R32 | V. Kallinen and L. Juvela, *De-crackling Virtual Analog Controls with Asymptotically Stable Recurrent Neural Networks*, 2025. [Preprint](https://arxiv.org/abs/2509.15622) | M; recurrent-model stability under control changes, not a proof for arbitrary networks. |
| R33 | *Parameter Estimation via Differentiable Modal Plate Synthesis*, DAFx 2026 Challenge, Task A. [PDF](https://dafx.de/paper-archive/2026/papers/DAFx26_challenge_77.pdf) | T; six-parameter estimation from eight synthetic IRs in this challenge, not universal identification of real plates. |
| R34 | E. Berdahl and D. Harris, *Frequency Shifting for Acoustic Howling Suppression*, DAFx 2010. [PDF](https://www.dafx.de/paper-archive/2010/DAFx10/BerdahlHarris_DAFx10_P101.pdf) | T; frequency shifting in acoustic feedback, distinct from proportional pitch shifting. |
| R35 | *Physical Modelling of a Wah-Wah Effect Pedal as a Case Study for Application of the Nodal DK Method to Circuits with Variable Parts*, DAFx 2011. [Archive](https://www.dafx.de/paper-archive/details/hR2kI9NQ8b1kjRgzxNk0pw) | M; circuits with variable parameters. |
| R36 | C. Gnegy and K. Werner, *Digitizing the Ibanez Weeping Demon Wah Pedal*, DAFx 2015. [Archive](https://dafx.de/paper-archive/details/1jzmFOCPECrcGgYQZxF6gw) | M; a concrete wah case for comparison with the generic FX11 baseline. |
| R37 | *Virtual Analog Buchla 259 Wavefolder*, DAFx 2017. [PDF](https://www.dafx.de/paper-archive/2017/papers/DAFx17_paper_82.pdf) | T; wavefolder circuit and reduction; test aliases under strong drive. |
| R38 | *Wave Digital Modeling of the Output Chain of a Vacuum-Tube Amplifier*, DAFx 2009. [Archive](https://www.dafx.de/paper-archive/details/xxUU9Xp4rpJLDYqM5dMOSg) | M; output stage, transformer and load interaction. |
| R39 | *Real-Time Wave Digital Filter Simulation of Cascaded Vacuum Tube Amplifiers Using the Modified Blockwise Method*, DAFx 2018. [PDF](https://www.dafx.de/paper-archive/2018/papers/DAFx2018_paper_25.pdf) | M; solving interactions between cascaded amplifier stages. |
| R40 | *Nonlinear Modeling of a Guitar Loudspeaker Cabinet*, DAFx 2008. [Archive](https://www.dafx.de/paper-archive/details/AG5zvKHmRq14rK8R4Uyvew) | M; limitations of representing a cabinet only with an IR. |
| R41 | *Optimal Filter Partitions for Real-Time FIR Filtering Using Uniformly-Partitioned FFT-Based Convolution*, DAFx 2011. [Archive](https://www.dafx.de/paper-archive/details/fWi-8K36aSnULRLT-W7z2A) | M; partition selection and computational cost. |
| R42 | E. Battenberg and R. Avizienis, *Implementing Real-Time Partitioned Convolution Algorithms on Conventional Operating Systems*, DAFx 2011. [Archive](https://dafx.de/paper-archive/details/S8X2gFWs-CsSUhrhJ7KeWQ) | M; scheduling and processing-load peaks. |
| R43 | *Optimization of Convolution Reverberation*, DAFx 2020. [Archive](https://www.dafx.de/paper-archive/details/Rj5F378RFBW38OU_AcbnDA) | M; additional convolution optimisation reading. |
| R44 | J. O. Smith, *The Leslie*, chapter de *Physical Audio Signal Processing*. [Text](https://www.dsprelated.com/freebooks/pasp/Leslie.html) | T; Doppler, amplitude and motion. A chapter of R02, not an independent publication. |
| R45 | *Improved PVSOLA Time-Stretching and Pitch-Shifting for Polyphonic Audio*, DAFx 2012. [Archive](https://www.dafx.de/paper-archive/details/2P-TV49ZRjU-FPFzwQXTZw) | M; hybrid pitch/tempo methods and transient handling. |
| R46 | *DAFX*, 2nd ed., chapter 7: time–frequency processing. [Materials](https://www.dafx.de/DAFX_Book_Page_2nd_edition/chapter7.html) | T; STFT and phase-vocoder materials; part of R01. |
| R47 | *A New Approach to Transient Processing in the Phase Vocoder*. [Archive DAFx](https://www.dafx.de/paper-archive/details/svqqbE0vLRFuz4BsmGhoDg) | M; transient assessment beyond sustained tones. |
| R48 | *DAFX*, 1st ed., chapter 7: time-segment processing. [Materials](https://www.dafx.de/DAFX_Book_Page/chapter7.html) | T; time segmentation and buffer effects; a different edition from R46. |
| R49 | *DAFX*, 2nd ed., chapter 8: source–filter processing. [Materials](https://dafx.de/DAFX_Book_Page_2nd_edition/chapter8.html) | T; vocoder, LPC and source–filter materials; part of R01. |
| R50 | *DAFX*, 2nd ed., chapter 10: spectral processing. [Materials](https://www.dafx.de/DAFX_Book_Page_2nd_edition/chapter10.html) | T; spectral-processing materials; part of R01. |
| R51 | *A Spectral Subtraction Rule for Real-Time DSP Implementation of Noise Reduction in Speech Signals*, DAFx 2009. [PDF](https://dafx.de/paper-archive/2009/papers/paper_49.pdf) | M; restoration reference; transfer from speech to music requires separate validation. |

### 10.3 Software: what to study and what not to assume

The licences below are indications observed in repository metadata/documentation, not a complete audit of every file, dependency or asset. Pin a commit and inventory before incorporation. No sonic superiority or measured runtime performance is attributed to software not executed in this research.

| ID | Software / official source | Use in the plan | Licence and evaluation state |
| --- | --- | --- | --- |
| S01 | [chowdsp_wdf](https://github.com/Chowdhury-DSP/chowdsp_wdf) | C++ WDF components, ports and adaptors | BSD-3-Clause; D/G |
| S02 | [AnalogTapeModel / CHOW Tape](https://github.com/jatinchowdhury18/AnalogTapeModel) | Implementation associated with tape research | GPL-3.0; G; study reference, not MIT code to copy |
| S03 | [BYOD](https://github.com/Chowdhury-DSP/BYOD) | Effect graphs, circuits and composition | GPL-3.0; G |
| S04 | [ChowMatrix](https://github.com/Chowdhury-DSP/ChowMatrix) | Creative delay networks and topologies | BSD-3-Clause; G |
| S05 | [Faust delays.lib](https://faustlibraries.grame.fr/libs/delays/) | Fractional delays, taps and DSP prototypes | D; inspect the library file and selected version’s licence |
| S06 | [Faust reverbs.lib](https://faustlibraries.grame.fr/libs/reverbs/) and [phaflangers.lib](https://faustlibraries.grame.fr/libs/phaflangers/) | FDN, diffusion and modulation | D; do not infer library licensing from the compiler licence |
| S07 | [Faust compressors.lib](https://faustlibraries.grame.fr/libs/compressors/) | Compression and envelope baselines | D; per-file licence review |
| S08 | [Csound hilbert](https://csound.com/manual/opcodes/hilbert/) | Quadrature and frequency-shifting examples | D; check the selected Csound version and example-material licences |
| S09 | [Faust tonestacks.lib](https://faustlibraries.grame.fr/libs/tonestacks/) | Tone stacks and circuit-derived coefficients | D; per-file licence review |
| S10 | [Faust vaeffects.lib](https://faustlibraries.grame.fr/libs/vaeffects/) | Virtual analog filters and circuit effects | D; per-file licence review |
| S11 | [fdnToolbox](https://github.com/SebastianJiroSchlecht/fdnToolbox) | MATLAB prototypes, matrices and FDN analysis | GPL-3.0; D/G; separate offline research tool |
| S12 | [ngspice](https://ngspice.sourceforge.io/docs.html) | Independent circuit reference | D; inspect distribution notices and record device-model versions |
| S13 | [mxrphase90](https://github.com/polimi-ispl/mxrphase90) | Supplementary material linked by R11 | Referenced by the paper; code and licence still require audit |
| S14 | [Klippel SIM](https://docs.klippel.de/db-lab/latest/simulation-auralisation/sim2/sim2.html) | Transducer nonlinearity definitions and simulation | D; commercial software, not an open library for incorporation |
| S15 | [pyroomacoustics](https://github.com/LCAV/pyroomacoustics) | Room/IR generation and reflection comparison; not a substitute for RT convolver tests | MIT; G and documentation consulted |
| S16 | [IEM Plug-in Suite](https://plugins.iem.at/) | Functional reference for Ambisonics and binaural processing | D; inspect source/asset licences for the selected version |
| S17 | [IEM plug-in descriptions](https://plugins.iem.at/docs/plugindescriptions/) | RoomEncoder, binaural and granular requirements/workflows | D; documentation for the same project as S16 |
| S18 | [Signalsmith Stretch](https://github.com/Signalsmith-Audio/signalsmith-stretch) | C++ pitch/time stretch; sample counts and latency | MIT; D/G; evaluate quality on the project’s own corpus |
| S19 | [Rubber Band Library](https://breakfastquay.com/rubberband/) | Pitch/time-stretch comparator | D; GPL or commercial licensing depending on the chosen arrangement |
| S20 | [nablafx](https://github.com/mcomunita/nablafx) | Differentiable black-box/gray-box models | MIT repository; G; check datasets and weights separately |
| S21 | [RTNeural](https://github.com/jatinchowdhury18/RTNeural) | Real-time audio network inference | BSD-3-Clause; G; does not itself solve training, stability or rate dependence |
| S22 | [NeuralAmpModelerCore](https://github.com/sdatkinson/NeuralAmpModelerCore) | Amplifier inference comparator | MIT core; G; captures and model files have separate provenance |
| S23 | [chowdsp_utils](https://github.com/Chowdhury-DSP/chowdsp_utils) | DSP utilities, modes, reverb, SIMD and tests | D; per-module licences include BSD-3-Clause and GPL-3.0 |

### 10.4 Minimum reading sequence

Start with R01/R02 and exercises N01–N03. Read R04/R03 with FX10/11; R06/R07 with N04 and FX16; R12/R13 before choosing antialiasing. Study R08/R09 with tape/BBD and R23–R25 with dynamics. For reverb, read R14, then R15/R17, then R19–R22. Use R46/R48–R50 for pitch, granular and spectral processing. Read R29/R30/R32 after establishing a baseline and identification protocol.

Recent work refines specific problems. Simple models remain essential for understanding states, stability, units and computational cost.
