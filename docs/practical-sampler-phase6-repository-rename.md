# Practical Sampler Phase 6 — Repository and Root Rename

Date: August 16, 2026
Status: Local implementation complete — Gate G6 held on hosted origin and inherited full-suite hangs
Depends on: Phase 5 / Gate G5 (passed by representative Salamander Piano REAPER recall)

## Outcome

The product repository now has a qualified checkout at
`E:/Dev/Cpp/VST/DecentRhapsody/PracticalSampler`. The original checkout had no configured Git
remote. Windows would not rename that open workspace directory in place, so the change used the
safe operational equivalent: commit the rename preparation, create a clean non-local clone at the
final path, verify the clone, and detach its temporary local-source remote. The old checkout remains
at commit `8a8a76794fcfe4066c4643c8acbd3718eaa6f50b` as the explicit rollback point.

The final checkout has no configured remote and no active source, test, workflow, tool, CMake cache,
or repository URL that depends on the old root path. Generated build output was recreated from
scratch under `PracticalSampler` rather than moved from the old checkout.

Gate G6 is not yet passed. The hosted `PracticalSampler` origin does not exist or is not configured,
so an origin-based clean clone cannot be proven. In addition, the inherited Debug CTest suite still
hangs in GUI-linked child-process shutdown. These are explicit operational blockers rather than
repository-name failures.

## Repository and path changes

- Changed the active repository/root presentation to `PracticalSampler` in the README and identity
  baseline.
- Removed the old absolute workspace path from `WavImportHostValidationTests.cpp`; its missing-local
  scenario now uses a repository-independent absent-media path.
- Updated identity-audit rules for the `PracticalSampler` workspace and excluded the retained legacy
  checkout used solely as the Phase 6 rollback point.
- Updated the DRSWeb reference synchronizer to read from `PracticalSampler` and publish under
  `protected-content/reference/PracticalSampler`.
- Updated the initiative-plan link to the Phase 5 report under the renamed root.
- Preserved CMake project/target names, source symbols, `drs*` identifiers, plug-in codes, VST3 CIDs,
  native extensions, schemas, and host parameter IDs.

## Clean-root audit

The following checks pass in the final checkout:

- Git worktree root is `E:/Dev/Cpp/VST/DecentRhapsody/PracticalSampler`.
- No Git remote contains the old path; no remote is configured pending hosted-repository creation.
- Active source/tool/test/workflow hard-coded old absolute-root hits: `0`.
- Old repository URL hits: `0`.
- CMake caches containing the old root: `0`.
- Exact presentation-identity ledger: `221` classified occurrences, `0` unclassified, `0` `CHANGE`,
  `0` `REGENERATE`, and complete owner/reason fields.
- The stable technical-identity verifier passes with project `DecentRhapsodyStudio`, plug-in codes
  `Dcrh` / `Drs0`, CIDs `ABCDEF019182FAEB4463726844727330` and
  `ABCDEF011234ABCD4463726844727330`, formats `.drsproj`, `.drinst`, `.drstrm`, `.drpkg`, and 16 host
  parameters.

## Clean builds and tests

### Debug

The clean `vs2022-debug` preset configured under the new root, and the `drs_all_tests` build target
completed its 2,167-step build. CTest then started its 198-test suite:

- `drs.phase0.smoke` passed.
- `drs.host_state.vst3_qualification` spawned its child process but did not terminate.
- A separate focused run passed `drs.phase0.smoke` and `drs.package_v2.records`, then reproduced the
  same shutdown hang in `drs.wav_import.host_validation`.

Both hung runs were terminated after confirming that their parent and child processes remained
responsive but made no progress. Neither test defines a CTest timeout. This is the inherited
full-suite lifecycle problem already observed before the repository rename.

### Release

The clean `vs2022-release` preset configured and built the standalone, VST3, smoke target, and focused
state/package qualification targets. The focused Release CTest matrix passed 7 of 7 tests in 15.52
seconds:

1. `drs.phase0.smoke`
2. `drs.host_state.vst3_qualification`
3. `drs.package_v2.records`
4. `drs.phase1.state_recall`
5. `drs.performance_package.export_lifecycle`
6. `drs.phase1.performance_package_host_validation`
7. `drs.phase1.performance_package_release_gate`

## Standalone and host smoke

The clean Release standalone launched from the renamed root, responded with the window title
`Practical Sampler - No Project Loaded`, and closed normally.

The isolated REAPER qualification also passed against the Release VST3 from the renamed root:

- one scanned bundle: `Practical Sampler.vst3`;
- host name: `VST3i: Practical Sampler (Practical Sampler Project)`;
- stable component CID: `ABCDEF019182FAEB4463726844727330`;
- saved/reopened automation values match;
- editor open and closed states pass; and
- duplicate instances remain independent.

The refreshed machine-readable host evidence is
`validation/reaper/phase5-identity-evidence/phase5-identity-summary.json`.

Release file metadata reports product `Practical Sampler` and company
`Practical Sampler Project` for both artifacts. The Release SHA-256 values are:

- standalone: `761E7DFBEDF80BAA740E0148B4906603356DAB0E5F3BD0EE91262F556001F57B`;
- VST3 module: `0D842F2C677DFA335706314A4746D5644027824E1D8A12ED7DE93BD276E0FA48`.

## Gate G6 decision

Local repository/root implementation and renamed-root product qualification are complete. G6
remains held until both of these conditions are met:

1. Rename or create the hosted repository as `PracticalSampler`, configure its origin, and repeat the
   clean-clone proof from that hosted origin.
2. Repair or bound the GUI-linked child-process shutdown hangs so the entire Debug and Release CTest
   suites can complete rather than relying on the passing focused Release matrix.

The old checkout is a rollback copy only. New development should use the `PracticalSampler` root.
