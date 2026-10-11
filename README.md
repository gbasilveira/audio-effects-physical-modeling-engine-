# Audio Effects Physical Modeling Engine

Research and plans for a modular audio effects engine combining physical models, virtual analog modelling and signal DSP. The documentation is organised around deriving, implementing and validating each mathematical model.

**Status:** planning documented; the C++ engine is not yet implemented. Coverage includes 32 effect families, 8 mathematical foundations, 17 implementation phases, 51 bibliography entries and 23 software/documentation entries. Some entries reference chapters or documentation within the same project; these are not 74 independent publications/products.

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
