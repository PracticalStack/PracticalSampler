# Practical Sampler identity initiative — Phase 3 tooling, installer, and tests

Date: August 15, 2026  
Status: Implementation complete — Gate G3 held on the clean full-suite baseline  
Depends on: Phase 2 / Gate G2

## Outcome

Active Windows build, packaging, install, smoke, VST3 qualification, and REAPER validation paths
now locate and present `Practical Sampler` artifacts. Installer and host vendor presentation uses
`Practical Sampler Project`. Compact CMake targets, `drs*` identifiers, bundle IDs, plug-in codes,
VST3 CIDs, native formats, schemas, and CTest names remain unchanged.

The Phase 3 implementation and all identity-specific qualification are complete. Gate G3 is not
marked passed because a true clean Debug run exposed identity-neutral runtime failures, timeouts, and
missing dependencies in the existing `drs_all_tests` aggregate. Those failures are documented
below rather than being hidden or expanded into an out-of-scope engine repair.

## Tooling and artifact discovery

- Windows bootstrap, installer-build, and direct VST3-install messages use Practical Sampler.
- Packaging and direct install resolve the canonical
  `drs_plugin_bundle_artefacts/<Configuration>/VST3/Practical Sampler.vst3` output.
- The tools reject a stale `Decent Rhapsody Studio.vst3` anywhere in the active build application
  tree, preventing ambiguous same-CID qualification.
- The direct installer removes an installed legacy bundle before copying the new bundle and checks
  that destructive bundle paths are direct children of the system VST3 directory.
- Stale old-name build products that CMake could no longer enumerate after the rename were removed
  from the Debug and Release output trees.

## Installer

The tester installer now presents:

| Field | Value |
|---|---|
| Product / plug-in | `Practical Sampler` |
| Publisher / company | `Practical Sampler Project` |
| Standalone | `Practical Sampler.exe` |
| VST3 destination | `Practical Sampler.vst3` |
| Installer filename | `PracticalSampler-Setup-0.1.0-phase3.exe` |

The existing Inno Setup `AppId` is preserved for deterministic upgrade continuity. An
`[InstallDelete]` rule removes `Decent Rhapsody Studio.vst3` before the same-CID new bundle is
installed. A silent clean install and a second install with an elevated legacy-bundle sentinel both
passed; the new standalone and VST3 remained installed and the legacy bundle was absent afterward.

## Tests and generated host fixtures

- All 103 JUCE test `PRODUCT_NAME` declarations derive from `DRS_PRODUCT_DISPLAY_NAME`.
- Test executable target names and CTest names remain unchanged.
- `drs_all_tests` now includes the registered VST3 host-state qualification target, closing the
  clean-build gap found when CTest could register that test without building its executable.
- Smoke and real VST3 host-state qualification resolve `Practical Sampler.vst3` and assert the new
  scanned plug-in name.
- REAPER baseline preparation and state injection use Practical Sampler.
- The baseline, scanner cache, vendor/category tags, six standard host scenarios, and three WAV
  startup scenarios were regenerated against only the repository's new-name bundle.
- Active REAPER project/chunk/cache fixtures contain `Practical Sampler (Practical Sampler Project)`
  and retain component CID `ABCDEF019182FAEB4463726844727330`.

## Passing qualification

| Check | Result |
|---|---|
| Clean Debug compile through `drs_all_tests` aggregate | PASS — generated product resources and new-name artifacts rebuilt. |
| Clean Release application, VST3, native content contract, and smoke build | PASS. |
| Phase 0 smoke / VST3 scan | PASS, 6.91 s. |
| Real VST3 host-state round trip | PASS, 2.41 s. |
| `drs.package_v2.records` | PASS, 0.27 s. |
| `drs.phase1.state_recall` | PASS, 4.24 s. |
| `drs.performance_package.export_lifecycle` | PASS, 2.69 s. |
| `drs.phase1.performance_package_host_validation` | PASS, 9.49 s. |
| Nine isolated REAPER scenario captures | PASS. |
| Installer compile | PASS — 9,862,037-byte Phase 3 installer. |
| Silent clean install | PASS. |
| Simulated legacy same-CID upgrade cleanup | PASS. |
| Technical identity baseline verifier | PASS. |
| Rolling identity audit | PASS — 322 occurrences, zero unclassified and zero `REGENERATE`. |

## Full-suite baseline preventing G3

The required clean full CTest run is not green independently of the naming surfaces:

- `drs.phase1.prepared_playback` blocks in its final migrated-zone checks;
- `drs.phase1.prepared_playback_worker` and `drs.sprint4.shell_cutover` terminate with access
  violations;
- Sprint 4 activation-payload and realtime-guard assertions fail;
- `drs.sprint4_entry.shell_parity` exceeds a 180-second timeout;
- several registered performance-engine tests are absent after a clean `drs_all_tests` build,
  showing that the aggregate has additional missing dependencies;
- `drs.phase1.performance_package` exceeds a 120-second focused timeout.

These failures do not inspect product/vendor names and occur after the identity-sensitive smoke and
VST3 tests pass. Repairing them would expand Phase 3 into runtime/concurrency work, so they are a
separate prerequisite for closing G3.

## Gate G3 decision

Phase 3 implementation is complete, including deterministic installer upgrade behavior and
new-name host fixtures. Gate G3 remains held until the clean full CTest baseline is repaired and
passes. No technical target or serialized contract was renamed.
