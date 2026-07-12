# Windows Build

This repository is Windows-first for Phase 0.

## Supported baseline

- Visual Studio 2022 Community
- `Desktop development with C++` workload
- MSVC `v143`
- CMake `3.28+`
- Windows SDK `10.0.22621.0+`
- Git

## What builds today

The current bootstrap builds:

- a product-owned HISE plugin-frontend compile probe
- a JUCE standalone shell target
- a JUCE VST3 shell target
- a `drs_phase0_smoke_tests` executable for startup and runtime initialization validation

The product-owned `engine_adapter` is present and compiled. HISE is vendored in `third_party/hise`, but it is not yet integrated into the CMake build as a full runtime because HISE itself is still driven by Projucer rather than a native top-level CMake flow.

The current HISE-backed handshake in this bootstrap has two parts:

- a configure-time vendor probe that compiles exact HISE snapshot metadata and readiness signals into the shell UI
- a product-owned plugin-frontend profile bridge that links generated frontend macros and lightweight HISE build metadata into `drs_engine_adapter`
- a product-owned HISE content resolver rooted at `content/hise_project/` for preset and asset path discovery
- a dedicated compile probe target that validates that linked bridge seam in isolation

The bootstrap uses the Visual Studio 2022 MSVC toolchain through `VsDevCmd.bat`, but generates with `Ninja` rather than the Visual Studio solution generator.

## Quick start

From the repository root:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\bootstrap-windows.ps1
```

The script enters the Visual Studio developer environment automatically via `VsDevCmd.bat`, so it can be run from a normal PowerShell session.

For a Release build:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\bootstrap-windows.ps1 -Configuration Release
```

To build and run the Phase 0 smoke tests:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\bootstrap-windows.ps1 -RunTests
```

To install the built VST3 bundle into the standard Windows VST3 location after a successful build:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\install-vst3-windows.ps1
```

To configure only:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\bootstrap-windows.ps1 -SkipBuild
```

## Direct CMake usage

Configure:

```powershell
cmake --preset vs2022-debug
```

Build:

```powershell
cmake --build --preset build-debug --target drs_hise_frontend_plugin_probe DecentRhapsodyStudioApp DecentRhapsodyStudioPlugin_VST3 drs_phase0_smoke_tests
```

Run tests:

```powershell
ctest --preset test-debug
```

## Generated output

The build presets write build trees under:

- `build/vs2022-debug/`
- `build/vs2022-release/`
 
These names refer to the VS2022 toolchain choice, not to the Visual Studio solution generator.

## Current limitation

This bootstrap is intentionally honest about the current seam:

- JUCE is built directly through CMake.
- HISE is vendored and documented.
- The selected HISE plugin frontend now compiles in an isolated probe and links lightweight frontend-profile metadata through a product-owned bridge seam.
- The smoke-test baseline validates that the current shell and engine bootstrap can be instantiated without immediately failing.
- The actual HISE-backed runtime handshake remains a later Phase 0 integration task.

For host-load workflow details, see `docs/host-validation.md`.
