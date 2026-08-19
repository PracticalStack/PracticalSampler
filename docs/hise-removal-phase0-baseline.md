# HISE Removal — Phase 0 Baseline and Native Content Contract

Status: Phase 0 in progress  
Date: 2026-08-19  
Scope: `PracticalSampler/` only. `_analysis/Rhapsody/` is explicitly excluded.

## Purpose

This note records the starting state for removing HISE from Practical Sampler and defines the
native content boundary that later removal work will use. HISE remains present during this phase so
the baseline can be compared against the post-removal build.

## Baseline environment

- Windows 11 workspace
- Visual Studio 2022 Community, MSVC `v143`
- CMake `4.3.0`
- Ninja supplied by the Visual Studio CMake installation
- Debug build directory: `build/vs2022-debug`
- Existing configure cache: `DRS_ENABLE_HISE_INTEGRATION=OFF`

The build must be invoked from the Visual Studio developer environment. Running CMake with the
ordinary PowerShell environment failed before compilation because MSVC could not locate
`stddef.h`; the supported `VsDevCmd.bat -arch=amd64` environment resolved that toolchain issue.

## Baseline results

Command used from `PracticalSampler/`:

```text
call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" -arch=amd64
cmake --build build/vs2022-debug --target drs_all_tests -j 2
```

Result: PASS. The build completed all 83 requested build steps, including the current HISE frontend
probe target and the native test executables.

CTest command:

```text
ctest --test-dir build/vs2022-debug --output-on-failure
```

Observed result: the test plan contains 200 tests. `drs.phase0.smoke` passed in 8.33 seconds. The
run then entered `drs.host_state.vst3_qualification`, which remained stalled in this headless
session and was stopped. The VST3 qualification needs a separately controlled host/timeout run
before this baseline can be called fully green.

The controlled retry used `ctest --test-dir build/vs2022-debug --output-on-failure -R
drs\\.host_state\\.vst3_qualification --timeout 30`. It reproduced the issue as an explicit
30.05-second CTest timeout with no orphaned qualification process left behind.

After adding the native content contract, the incremental all-test build also completed
successfully: 232 build steps, including all test resource targets and final test executables. The
focused `drs.phase0.smoke` rerun passed in 6.41 seconds.

## Current HISE migration inputs

The temporary product-owned HISE content tree contains 26 files:

### Reusable source assets to migrate later

- `content/hise_project/Samples/DRS_Sine_A3.wav`
- `content/hise_project/Samples/DRS_TriangleLead_A4.wav`
- `content/hise_project/AudioFiles/DRS_Room.wav`
- `content/hise_project/Images/drs-mark.svg`
- `content/hise_project/Images/drs-stage.svg`, if the native UI still needs the artwork

### HISE-only assets to remove later

- `project_info.xml` and `user_info.xml`
- `SampleMaps/`
- `Scripts/`
- `UserPresets/`
- `XmlPresetBackups/`
- HISE folder scaffolding and `.gitkeep` files
- the HISE-specific content README

No HISE content is deleted in Phase 0. The classification is recorded so Phase 2 can migrate only
reusable assets and avoid carrying HISE authoring semantics into the native runtime.

## Native content contract

Phase 0 establishes these product-owned roots:

| Root | Purpose | Ownership |
| --- | --- | --- |
| `content/samples/` | Source audio and future native sample manifest | Product-owned, HISE-independent |
| `content/runtime/` | Native project, instrument, package, and negative-corpus fixtures | Product-owned, schema-driven |
| `content/hise_project/` | Temporary migration input only | Scheduled for Phase 2 removal |

The adapter now exposes `drs::engine::getNativeContentRoots()` through
`engine_adapter/include/drs/engine/NativeContent.h`. It returns absolute generic-separator paths
for the repository root, native samples root, and Phase 1 runtime root. New native code should use
this contract rather than embedding `content/hise_project` or walking HISE folder conventions.

The native sample contract is documented in `content/samples/README.md`. Native fixtures may refer
to sample IDs and relative paths, but they must not depend on HISE sample maps, scripts, presets, XML
backups, or HISE-specific metadata.

## Independent dependency audit

- The only non-HISE code reference found for RTNeural is an include-directory inheritance from
  `third_party/hise/hi_tools/hi_neural/RTNeural/modules` in `engine_adapter/CMakeLists.txt` and
  repeated test-target include paths in `tests/CMakeLists.txt`.
- No product-owned source include of an RTNeural header was found outside the HISE vendor tree.
- Phase 1 should remove those inherited include paths and confirm the targets still compile. A new
  direct RTNeural dependency is not justified by the Phase 0 scan.
- The nested JUCE under `third_party/hise/JUCE/` is not a product dependency; the supported project
  already has `third_party/juce/` as its direct JUCE source.

## Phase 0 exit criteria

- [x] Baseline build captured from the supported Visual Studio environment.
- [x] Native samples root created and documented.
- [x] HISE content files classified into reusable assets and HISE-only authoring data.
- [x] Native content path API added to the adapter.
- [x] Independent RTNeural usage audited.
- [ ] Controlled VST3 qualification baseline completed with an explicit timeout.
- [ ] Phase 1 removal work started only after this baseline is accepted.
