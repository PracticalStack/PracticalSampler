# Practical Sampler identity initiative — Phase 1 runtime and build presentation

Date: August 15, 2026  
Status: Complete — Gate G1 passed  
Depends on: Phase 0 / Gate G0

## Outcome

The standalone application and VST3 now present the approved identity:

| Field | Value |
|---|---|
| Product / plug-in name | Practical Sampler |
| Company / vendor name | Practical Sampler Project |
| Standalone artifact | `Practical Sampler.exe` |
| VST3 artifact | `Practical Sampler.vst3` |

No product branding, logo, image, internal target rename, source-tree rename, format change, or schema
change is included.

## Implementation

- Added `DRS_PRODUCT_DISPLAY_NAME` and `DRS_COMPANY_DISPLAY_NAME` at the top-level CMake boundary.
- Applied those values to the JUCE standalone and VST3 `PRODUCT_NAME` / `COMPANY_NAME` metadata.
- Applied the values to the generated runtime display-name constants.
- Routed the standalone title fallback through the generated product display-name constant.
- Changed the compatibility-copy destination to
  `DecentRhapsodyStudioPlugin_artefacts/<config>/VST3/Practical Sampler.vst3`; the compatibility
  target and directory names remain unchanged.
- Left the compact `DecentRhapsodyStudio` JUCE settings storage keys unchanged.

## Built artifact evidence

The Debug standalone and VST3 targets built successfully after an x64 Visual Studio developer-shell
rebuild.

| Artifact | Windows product metadata | SHA-256 |
|---|---|---|
| `build/vs2022-debug/app/DecentRhapsodyStudioApp_artefacts/Debug/Practical Sampler.exe` | Product / description: Practical Sampler; company: Practical Sampler Project; version: 0.1.0 | `0460E5EDF34EEF043DE45D0C0FA042AD07B251CA8BBED73A2880C9EEDC4AA98D` |
| `build/vs2022-debug/app/drs_plugin_bundle_artefacts/Debug/VST3/Practical Sampler.vst3/Contents/x86_64-win/Practical Sampler.vst3` | Product / description: Practical Sampler; company: Practical Sampler Project; version: 0.1.0 | `F7B862ADFBBF22033B2A69542632C0E12A3170A2EDA162979D727E3433E5A9A7` |

The canonical and `DecentRhapsodyStudioPlugin_artefacts` compatibility-copy bundles have identical
module metadata and binary hashes.

JUCE's VST3 manifest helper loaded the built module and generated `moduleinfo.json` with:

- module and class name `Practical Sampler`;
- factory and class vendor `Practical Sampler Project`;
- component CID `ABCDEF019182FAEB4463726844727330`;
- controller CID `ABCDEF011234ABCD4463726844727330`.

The standalone was launched in a hidden validation session, produced a responsive native window,
and reported the runtime title `Practical Sampler - No Project Loaded`.

## Stable identity proof

`tools/verify-practical-sampler-technical-identity.ps1` passes against the Phase 0 baseline and the
new built VST3 metadata. The following remain unchanged:

- CMake project and all application, plug-in, compatibility, and test targets;
- application and plug-in bundle IDs;
- `Dcrh` manufacturer code and `Drs0` plug-in code;
- VST3 component and controller CIDs;
- `.drsproj`, `.drinst`, `.drstrm`, and `.drpkg` extensions;
- package signatures, schema contracts, and all 16 host parameter IDs;
- compact `DecentRhapsodyStudio` application-settings identity.

The rolling identity audit reports 540 remaining legacy presentation-string occurrences and zero
unclassified occurrences. Phase 1 removed 12 `CHANGE` occurrences from its owned source surfaces.

## Generated-output note

JUCE's generated Windows `.rc` custom command does not declare its generated `Info.txt` as a build
dependency. After CMake adopted the new metadata, the two stale generated `.rc` files were removed
and regenerated before the final link. The rebuilt resources now contain the approved product and
company names.

Stale old-name Debug executables, symbols, shared-code library, and VST3 bundle directories were
removed from the build tree after the renamed artifacts were verified. They were generated outputs
and can be recreated from a pre-rename source revision.

## Deferred by the delivery plan

`Phase0SmokeTests.cpp` and `Vst3HostStateQualificationTests.cpp` were subsequently updated with
the native artifact paths and active host fixtures during Phase 3. The Phase 1 gate evidence remains
the successful VST3 manifest load, direct module metadata inspection, Windows resource inspection,
runtime launch, and stable-ID verifier.

## Gate G1 decision

Gate G1 passes: the standalone and VST3 build with the approved visible product/company identity,
the runtime window title is correct, the canonical and compatibility-copy artifacts use the new
filename, and every compatibility-sensitive identifier matches the Phase 0 baseline.
