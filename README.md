# Audio Effects Physical Modeling Engine

A C++20 modular audio effects engine combining physical models, virtual analog modelling and signal DSP, with research and executable validation. The documentation is organised around deriving, implementing and validating each mathematical model.

**Status:** v0.1 implements a real-time mono/stereo core, graph, WAV renderer and baselines for all 32 effect families. Device-specific physical extensions and plug-in wrappers remain planned. See the [implementation report and coverage matrix](docs/implementation/README.md). Coverage includes 32 effect families, 8 mathematical foundations, 17 implementation phases, 51 bibliography entries and 23 software/documentation entries. Some entries reference chapters or documentation within the same project; these are not 74 independent publications/products.

## Build and run

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
build/audiofx-render --family diode --drive 4 --oversampling 4 --mix 1 --signal music --seconds 4 --output diode.wav
```

The C++ library has no external dependencies. `make test` additionally runs independent NumPy/SciPy comparisons after installing the verification requirements below. [API contracts, all models, mathematical choices and measured limitations](docs/implementation/README.md).

## Start here

- [Complete research report](docs/plan/research-and-plans.md) — equations, component plans, architecture, validation and sources.
- [Index and reading routes](docs/plan/README.md).
- [Literature and software survey](docs/plan/literature-survey.md).
- [Mathematical specification](docs/plan/engine-specification.md).
- [Implementation plan](docs/plan/implementation-plan.md).
- [Phases and acceptance criteria](docs/plan/phasing-plan.md), with a [YAML dependency graph](docs/plan/phases.yaml).

Families include delay, tape/BBD, flanger, chorus, phaser, EQ, wah, dynamics, distortion, amplification, speaker/cabinet, algorithmic and physical reverberation, spatial audio, pitch/time stretch, granular, vocoder, spectral processing, lo-fi, resonators, restoration and hybrid composition.

## Reproducible verification

```bash
python -m pip install -r docs/plan/verification/requirements.txt
python docs/plan/verification/check_models.py
python docs/plan/verification/check_plan.py
```

[Results of 12 mathematical experiments](docs/plan/verification/results.md), with versions and test limitations in [JSON](docs/plan/verification/results.json). These verify selected equation cases; they are not hardware measurements or real-time engine benchmarks.

## Maintenance and licensing

`docs/plan/research-and-plans.md` is the canonical text. Generate the four focused views with `python docs/plan/verification/build_views.py`. `phases.yaml` governs dependencies and coverage; the report’s FX cards govern mathematical content. Update both when scope changes.

The organisation follows [plucked-physical-modeling-engine](https://github.com/gbasilveira/plucked-physical-modeling-engine), without modifying that repository. Original code and text follow this repository’s [MIT licence](LICENSE). External references, libraries and data retain their own licences.
