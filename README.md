# Practical Sampler

Practical Sampler is an open-source sampler and instrument-building environment for importing sample
libraries, authoring playable instruments, auditioning changes, and publishing instruments for use in
a DAW.

The Windows-first codebase produces both a standalone application and a VST3 instrument plug-in. The
application and plug-in share the same product-owned sampler, authoring, import, DSP, streaming, and
package runtime.

## Project status

**Public V1:** Practical Sampler `1.0.0` is the first stable public release. Project, instrument,
and package compatibility is governed by the documented V1 contracts and compatibility policies.

The implemented V1 candidate includes a native sampler core, one-way SFZ conversion, layer/group/zone
authoring, waveform and loop editing, articulations and performance rules, instrument controls with
MIDI CC assignment, a scoped DSP graph, large-instrument streaming, editable projects, and exported
playable packages. Current work is focused on closing the documented SFZ compatibility gaps, deepening
articulation authoring, and making the DSP system broader and easier to extend.

The dependency-ordered plan from V1 through scripting lives in the
[V1 capability and engine roadmap](https://practicalsampler.com/practical-sampler-v1-roadmap).

## Implemented functionality

### Import, projects, and packages

- Batch WAV import and one-way SFZ conversion into a native editable project.
- Playback and streaming of supported WAV and FLAC sample sources; direct WAV sources support
  RIFF/RF64 PCM16/24/32 and float32 mono/stereo data.
- Bounded SFZ include/define processing and an import review that distinguishes converted,
  approximated, report-only, unsupported, and malformed data instead of silently discarding it.
- Native SFZ projection for key/velocity mapping, selection and sequence rules, playback regions,
  loops, tuning, controller modulation, keyswitch articulations, release behavior, pedal conditions,
  and compatible instrument-control metadata.
- Editable `.drsproj` save/reopen, schema migration, source validation, and recovery for missing or
  relocated media.
- The default user library is `Documents\PracticalSampler`; the standalone and plug-in remember the
  last-used SFZ import directory.
- Deterministic export and reopening of read-only `.drpkg` playable instruments, including large
  package-v2 streaming, authenticated records, host-locator recall, and optional license text.

### Instrument authoring

- A native `layer -> group -> zone` hierarchy with stable IDs, ordering, selection, scoped gain/pan,
  routing, and controller/velocity crossfade metadata.
- A scalable structure browser and map for large instruments, including search, overlap handling,
  multi-selection, zoom, pan, overview navigation, and focused audition.
- Non-destructive root key, key range, velocity range, round-robin, playback start/end, loop mode,
  loop range, and loop-crossfade editing.
- Waveform selection, frame snapping, zero-crossing assistance, bounded peak generation, and playback
  of the region, loop, or current selection.
- Transactional authoring with undo/redo and explicit dirty/saved state.
- Separate draft Preview and published Performance lanes, so incomplete edits can be auditioned
  without replacing the active playable instrument.

### Sampler and performance engine

- A shared immutable render model for standalone, VST3, authoring Preview, and published Performance,
  with isolated voice/event state and last-known-good activation behavior.
- Pitched sample playback with linear interpolation, sample-accurate event offsets, smoothed live
  tuning modulation, bounded voice allocation, and deterministic voice stealing.
- First-class articulations with latch keyswitch activation and consumption, plus declarative
  note-on, note-off, release, pedal, one-shot, choke, and round-robin routing.
- Sustain and continuous-damper behavior with deferred releases, repedalling, release samples, and
  deterministic same-offset event ordering.
- Streaming heads/pages and bounded caches for large source sets and multi-gigabyte packaged
  instruments; file and decode work stays outside the audio callback.
- Project recall, VST3 host-state recall, editor close/reopen, replacement recovery, and retained
  playback for the last valid generation.

### Instrument controls, mixing, and DSP

- Native Instrument Controls with stable IDs, defaults, labels, semantic targets, manual MIDI CC
  assignment, MIDI learn, channel scope, conflict handling, and undoable edits.
- Imported SFZ controller metadata and compatible `*_onccN` targets for gain, pan, tune, and amplitude
  envelope behavior; active gain, pan, and tuning changes use the sampler's smoothing path.
- Publishable Performance macros and mixer controls backed by versioned control-law mappings.
- Immutable, versioned DSP graphs at zone, group, layer, and instrument scope with validation,
  bypass/recovery behavior, bounded state/tail cost, and package persistence.
- Six implemented curated DSP algorithms: Gain, Saturator, Compact EQ, Chorus, Stereo Delay, and
  Algorithmic Reverb.

### Application and distribution

- Standalone application and VST3 instrument plug-in built with JUCE and CMake.
- Stable Practical Sampler product identity while preserving existing VST3 component IDs, host
  parameters, schemas, and settings continuity.
- REAPER-qualified plug-in scan, instance independence, automation/state round-trip, package recall,
  and editor-open/editor-closed lifecycle behavior.
- A Windows installer that packages the standalone app and VST3, cleans up legacy product
  artifacts during upgrade, and install to the standard application/VST3 locations.

## Current V1 boundaries

- SFZ support is an explicit and growing compatibility profile, not a claim of complete SFZ support.
  Vendor extensions and unsupported sound-critical semantics remain visible in the import report.
- Articulation activation is currently latch-keyswitch based. Momentary, toggle, CC-range, program
  change, composite conditions, transitions, and articulation-owned DSP remain roadmap work.
- Playable-package export currently rejects authored performance banks and non-default group/zone
  pan because those values are not yet preserved by the package projection.
- The current DSP pipeline and algorithms are functional and real-time bounded, but the algorithm
  catalog, modulation model, routing vocabulary, presets, and authoring contract still need deeper
  development.
- Scripting is not implemented. It is intentionally sequenced after stable articulation,
  modulation, DSP, event, persistence, and real-time budget contracts.
- Windows is the supported development and release target today.

## Validation

The configured Debug tree currently registers 221 CTest targets covering unit, contract,
integration, lifecycle, realtime-safety, responsiveness, package, standalone, VST3, and host
behavior. Separate qualification scripts cover installer upgrades and REAPER scenarios. Release
qualification also uses real instrument corpora where licensing and local availability permit it:

- Accurate Salamander: large WAV streaming/package export and continuous-damper qualification.
- jRhodes3d: FLAC, crossfade, round-robin, SFZ lifecycle, and cancellation coverage.
- SM Drums: permanent large-include/define and 3,358-region parser regression corpus.
- Naked Drums: FLAC playback plus imported mixer/control/MIDI-binding qualification.

These corpora demonstrate the tested compatibility boundary; they do not imply that every SFZ file
or dialect imports without findings.

## Tested instruments
https://www.ir.isas.jaxa.jp/~cyamauch/AccurateSalamander/  
https://sfzinstruments.github.io/pianos/jrhodes3d/  
https://sfzinstruments.github.io/drums/sm_drums/  


## Repository layout

- `app/` — standalone, VST3, and shared user-interface code
- `engine_adapter/` — sampler core, import, playback, streaming, and engine-facing boundaries
- `content/` — product-owned instruments, native samples, and runtime fixtures
- `tests/` — unit, integration, performance, lifecycle, and host-validation coverage
- `tools/` — build, packaging, installation, and validation scripts
- `docs/` — architecture, authoring, build, and release-evidence documentation
- `third_party/` — vendored dependencies, currently JUCE and nlohmann/json

## Building on Windows

The supported development baseline is Visual Studio 2022 with the Desktop development with C++
workload, MSVC v143, CMake 3.28 or newer, and Windows SDK 10.0.22621.0 or newer.

From a normal PowerShell session:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\bootstrap-windows.ps1 -RunTests
```

This configures the Debug tree, builds the application, plug-in, and registered `drs_all_tests`
targets, then runs the CTest preset. For a Release application and plug-in build without the full
test run:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\bootstrap-windows.ps1 -Configuration Release
```

To build the versioned installer from the Release outputs (requires Inno Setup 6):

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\build-tester-installer.ps1 -Configuration Release
```

Build output is written to `build/vs2022-debug/` or `build/vs2022-release/`. See
[`docs/windows-build.md`](docs/windows-build.md) for direct CMake commands, VST3 installation, and
installer instructions.

## Documentation

- [Architecture overview](docs/architecture-overview.md)
- [Windows build guide](docs/windows-build.md)
- [Host validation](docs/host-validation.md)
- [SFZ region, waveform, and loop authoring](docs/waveform-region-authoring-guide.md)
- [Instrument controls and MIDI CC](docs/instrument-controls-midi-cc.md)
- [Performance engine V1 contract](docs/performance-engine-v1-contract.md)
- [Continuous-damper qualification](docs/continuous-damper-hp05-qualification.md)
- [Layer hierarchy contract](docs/layer-contract-policy.md)
- [Curated DSP release contract](docs/curated-dsp-wave1-release.md)
- [Large-instrument streaming](docs/large-instrument-streaming-support.md)
- [Playable-package compatibility policy](docs/phase1-performance-package-compatibility-policy.md)
- [Licensing and distribution](docs/licensing.md)

Published instrument performance surface with authored macros and custom artwork

<img width="2868" height="1698" alt="image" src="https://github.com/user-attachments/assets/f52c4175-1adf-4e4b-b15e-7e50c648a8f7" />

Authoring workspace with Instrument Structure browser, Zone Map and Workbench
<img width="2870" height="1700" alt="image" src="https://github.com/user-attachments/assets/8f258184-a03e-4049-8a73-13167c17beb2" />

Macros at instrument, layer, group or zone level.
<img width="2820" height="732" alt="image" src="https://github.com/user-attachments/assets/7bf1399a-ebd5-45e2-bbe8-018a7770e21c" />

Routing at instrument, layer, group or zone level
<img width="2800" height="744" alt="image" src="https://github.com/user-attachments/assets/3fb727c0-5c11-4011-9e0b-ca0887b2e5c2" />



## License

Project-owned software is licensed under the
[GNU Affero General Public License v3.0 only](docs/licensing.md). Third-party code and separately licensed
content retain their own terms. See the [V1 licensing statement](docs/licensing.md) for scope,
dependency compatibility, content licensing, and distribution notes.
