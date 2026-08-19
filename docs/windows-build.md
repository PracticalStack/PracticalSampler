# Windows Build

This repository is Windows-first for the native runtime and shell.

## Supported baseline

- Visual Studio 2022 Community
- `Desktop development with C++` workload
- MSVC `v143`
- CMake `3.28+`
- Windows SDK `10.0.22621.0+`
- Git

## What builds today

The current bootstrap builds:

- a JUCE standalone shell target
- a JUCE VST3 shell target
- a `drs_phase0_smoke_tests` executable for startup and runtime initialization validation
- a `drs_native_content_contract_tests` executable for native content-root and fixture validation

The product-owned `engine_adapter` is present and compiled as the native runtime seam. Product source
samples are rooted at `content/samples/`, and runtime fixtures are rooted at `content/runtime/`.

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

To create a simple tester-facing `setup.exe` from the current build outputs:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\build-tester-installer.ps1 -Configuration Release
```

This helper expects Inno Setup 6 to be installed and packages the built VST3 bundle into the
standard VST3 location. If the standalone app artefacts are present, it also includes the
standalone executable in the installer.

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
cmake --build --preset build-debug --target DecentRhapsodyStudioApp DecentRhapsodyStudioPlugin drs_phase0_smoke_tests drs_native_content_contract_tests
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

This bootstrap is intentionally honest about the current native seam:

- JUCE is built directly through CMake.
- Native runtime and content contracts are built directly through the product-owned adapter.
- Product samples resolve from `content/samples/`, while runtime fixtures resolve from `content/runtime/`.
- The smoke-test baseline validates that the current shell and engine bootstrap can be instantiated without immediately failing.
- Host qualification remains a separate validation step after native startup succeeds.

For host-load workflow details, see `docs/host-validation.md`.
