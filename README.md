# Practical Sampler

Practical Sampler is a sampler and instrument-building environment in development for musicians,
sound designers, and instrument developers.

The goal is to build a **first-class open-source sampler and instrument builder** with comprehensive
authoring tools: one application for importing sample libraries, shaping playable instruments,
auditioning changes, and publishing instruments for use in a DAW.

## Project status

Practical Sampler is under active development and is not yet a stable public release. The current
Windows-first codebase builds a functional standalone application and VST3 plug-in. Its sampler
core, authoring workspace, import pipeline, project persistence, and playable-package workflow are
implemented and covered by an extensive automated test suite, including validation in REAPER.

Recent work has focused on scalable authoring and playback for large instruments, interactive SFZ
region and loop editing, bounded waveform rendering, reliable DAW state recall, and a consistent
desktop authoring experience. File formats, compatibility guarantees, and the final source license
may still change before the first public release.

## Key features

- Standalone application and VST3 instrument plug-in built with JUCE and CMake.
- A shared, product-owned sampler core for authoring preview and performance playback.
- WAV import plus one-way SFZ conversion with explicit reporting for unsupported or ambiguous data.
- Non-destructive zone, key-range, velocity, round-robin, playback-region, loop, and loop-crossfade
  authoring.
- A scalable Map and Waveform workspace with selection, zoom, pan, audition, undo, and redo.
- Groups, articulations, curated DSP routing, and publishable macro controls.
- Separate draft Preview and published Performance workflows, so edits can be auditioned safely
  before they replace the playable instrument.
- Streaming playback and bounded caches for large sample sets and packaged instruments.
- Project save/reopen, VST3 host-state recall, and resilient handling of missing or relocated media.
- Export and reopening of playable `.drpkg` instrument packages.
- Accessibility-aware keyboard navigation, focus handling, and control metadata throughout the
  authoring workspace.

## Repository layout

- `app/` — standalone, VST3, and shared user-interface code
- `engine_adapter/` — sampler core, import, playback, streaming, and engine-facing boundaries
- `content/` — product-owned instruments, runtime fixtures, and HISE project content
- `tests/` — unit, integration, performance, lifecycle, and host-validation coverage
- `tools/` — build, packaging, installation, and validation scripts
- `docs/` — architecture, authoring, build, and release-evidence documentation
- `third_party/` — vendored dependencies, including JUCE and HISE snapshots

## Building on Windows

The supported development baseline is Visual Studio 2022 with the Desktop development with C++
workload, MSVC v143, CMake 3.28 or newer, and Windows SDK 10.0.22621.0 or newer.

From a normal PowerShell session:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\bootstrap-windows.ps1 -RunTests
```

For a Release build:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\bootstrap-windows.ps1 -Configuration Release
```

Build output is written to `build/vs2022-debug/` or `build/vs2022-release/`. See
[`docs/windows-build.md`](docs/windows-build.md) for direct CMake commands, VST3 installation, and
tester-installer instructions.

## Documentation

- [Architecture overview](docs/architecture-overview.md)
- [Windows build guide](docs/windows-build.md)
- [Host validation](docs/host-validation.md)
- [SFZ region and waveform authoring](docs/waveform-region-authoring-guide.md)
- [Large-instrument streaming](docs/large-instrument-streaming-support.md)

Practical Sampler is being developed in the open, but contributor guidance and the final
open-source license have not yet been published.
