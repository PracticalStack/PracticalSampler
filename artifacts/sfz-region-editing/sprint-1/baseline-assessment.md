# SFZ Region Editing Sprint 1 Baseline

Date: 2026-08-17

Baseline commit: `cabef5cd`

Scope: before production waveform editing UI

## Existing waveform path

| Seam | Baseline behavior | Sprint 1 guardrail |
|---|---|---|
| Peak I/O | `buildWaveformPeaks` reduces the full source to 192 display points by default and reads through a bounded 4,096-frame buffer. | Preserve bounded worker-owned reads. Visible-range level-of-detail and tiling remain Sprint 2 work. |
| Service ownership | `WaveformPreviewService` owns a worker thread, supports cancel/supersede, and atomically publishes immutable snapshots. | Interaction policy remains pure and performs no source I/O. Do not move waveform reads into paint or pointer callbacks. |
| Paint | `WaveformDetailView` paints the surface, center line, fixed peak columns, and two loop markers. Its public contract is only `setPreview(...)` plus `paint(...)`. | No production component interaction is added in Sprint 1. Frame/pixel transforms are introduced as JUCE-independent policy. |
| Selection | The selected zone controls which cached/source waveform is requested. The waveform has no temporary range selection, handles, mouse methods, keyboard methods, or gesture transaction. | Define selection/range normalization and gesture terminal states without dirtying or mutating the project. |
| Preview dispatch | Selected-source preview work is asynchronous and newest-request-oriented; the current request carries project/source identity, file identity, 192 points, and a 4,096-frame chunk size. | A future drag may update only a transient overlay. Project edit and Preview preparation occur once after commit, never per pointer move. |
| Playable package | `PerformancePackageProjection` rejects any zone whose loop enabled/start/end values are non-default because the package projection does not preserve them. | Keep this rejection until the package-parity phase proves project → package → open/render preservation. |

## Sprint 1 contract baseline

- SFZ effective-opcode inheritance already exists in `normalizeSfzDocument`.
- Production SFZ projection previously copied `offset`, `loop_start`, and `loop_end` directly and flattened `loop_mode` into `loopEnabled`.
- The previous direct copy treated SFZ `loop_end` as if it were already exclusive. Sprint 1 now centralizes conversion so inclusive `loop_end=N` becomes native exclusive `N+1` exactly once.
- The current project model has no playback-end field. Sprint 1 validates and normalizes SFZ `end`, reports it as pending schema support, and does not falsely claim project persistence.
- The defined SFZ `end=-1` silent-region sentinel is represented explicitly and omitted from audible native projection rather than rejected or accidentally rendered.
- `one_shot` and `loop_sustain` are typed in the new region contract but remain honestly classified as compatibility approximations until the runtime model can preserve their distinct note-off lifecycle.

## Deterministic fixtures

`tests/fixtures/sfz-region-contract` now contains:

- inherited and region-local forms of all five baseline opcodes;
- `end=0` and a one-frame region;
- all four portable SFZ v1 loop modes;
- malformed frame values, an unsupported loop mode, and a reversed loop;
- a manifest for zero-length, one-frame, short mono, short stereo, multichannel, and deep 64-bit/long-source cases.

WAV loop fallback is supplied as typed source metadata in the pure tests. Later source-inspection work can populate the same contract without changing its precedence rules.

## Recorded Debug evidence

| Test slice | Result |
|---|---|
| `drs.sfz_region.contract` + `drs.waveform_region.policy` | PASS — 2/2, 0.10 s |
| Optimized `drs.sfz_region.contract` + `drs.waveform_region.policy` | PASS — 2/2, 0.14 s |
| Existing SFZ contract, normalization, compatibility, and projection | PASS — 4/4 |
| Existing waveform peak builder and preview service | PASS — 2/2 |
| Existing SFZ report model and runtime playback | PASS — 2/2, including the real fixture runtime path |

The focused test targets compile against C++17 and do not construct a JUCE editor. The initial uninitialized-shell build attempt could not locate MSVC standard headers; rerunning under the VS2022 developer environment built the same sources successfully.
