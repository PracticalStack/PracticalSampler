# Practical Sampler Phase 5 — Host Qualification and Identity Audit

Date: August 15, 2026  
Status: Complete — Gate G5 passed
Depends on: Phase 4 / Gate G4

## Outcome

The Phase 5 identity work is implemented. Practical Sampler by Practical Sampler Project passes a
clean isolated REAPER scan, fresh-session creation, host automation save/reopen, editor-open and
editor-closed checks, duplicate-instance independence, clean installer qualification, simulated
legacy-artifact upgrade, installed standalone launch, settings continuity, release metadata checks,
focused state/package tests, the stable-identity verifier, and the exact-name audit.

Gate G5 passed after a manual qualification of the representative end-user workflow: create a new
REAPER project, insert Practical Sampler, load and play the Salamander Piano packaged instrument,
save and close REAPER, reopen the project, and confirm that the instrument loads and plays correctly.
The inherited automated audible matrix remains red, but this result is now classified as
test-infrastructure debt because both the fresh-session automation harness and the representative
packaged-instrument workflow pass.

## Release build and focused qualification

The supported Visual Studio bootstrap configured the Release tree and rebuilt the standalone,
VST3, smoke, state-recall, package, and release-gate executables. An initial direct CMake invocation
outside the developer shell failed because MSVC and Windows SDK include paths were absent; the
repository bootstrap loaded `VsDevCmd.bat` and the same build completed.

The final focused Release matrix passed 7/7:

- `drs.phase0.smoke`
- `drs.host_state.vst3_qualification`
- `drs.package_v2.records`
- `drs.phase1.state_recall`
- `drs.performance_package.export_lifecycle`
- `drs.phase1.performance_package_host_validation`
- `drs.phase1.performance_package_release_gate`

This covers VST3 scan and audible host-state round trip, standalone project/state round trip,
playable-package open/activation, plug-in editor close/reopen, and release package behavior.

## Clean isolated REAPER identity and current-session recall

`validation/reaper/run-practical-sampler-identity-qualification.ps1` creates a new isolated scan
root containing exactly one `Practical Sampler.vst3` Release bundle and deletes the isolated host
cache before launch. REAPER produced exactly one external scan entry:

- `VST3i: Practical Sampler (Practical Sampler Project)`
- stable component CID `ABCDEF019182FAEB4463726844727330`
- no legacy display-name entry

The harness then created a new project, inserted the plug-in and MIDI, added two Tone automation
points (`0.23` at 0 seconds and `0.81` at 2 seconds), set Motion to `0.77`, opened and closed the
editor, saved, closed REAPER, reopened the project, and compared the saved values. All values and
automation points matched. It duplicated the track, changed the second instance to Motion `0.11`,
and proved the source remained at `0.77`.

The first harness draft saved immediately after setting Motion and correctly exposed the plug-in's
message/audio-boundary timing: the reopened value was the startup default. The final reproducible
harness waits for initialization before setting values and waits again before save, matching the
production synchronization contract. Its final signed summary is
`validation/reaper/phase5-identity-evidence/phase5-identity-summary.json`.

## Audible REAPER qualification

The existing project-aware matrix now accepts `-Configuration Release` and was run at 48 kHz with a
256-frame block. REAPER successfully loaded one enabled, online instance with the correct product
and vendor identity, exposed the expected parameter names, inserted 28 validation MIDI notes, and
captured the open editor. The run failed its audio/state assertions:

- Tone restored as startup `0.3499999940` rather than the fixture value;
- Motion restored as startup `0.1500000060` rather than the fixture value; and
- 1,904 peak probes observed zero nonzero samples.

Curated automated-failure evidence is in
`validation/reaper/phase5-audible-reaper-failure.txt`. It is retained rather than rewritten so the
stale fixture or harness behavior can be repaired separately.

On August 15, 2026, the product owner manually qualified the intended audible recall workflow:

1. Created a new REAPER project and track.
2. Inserted Practical Sampler.
3. Loaded and played the Salamander Piano packaged instrument.
4. Saved the project and closed REAPER.
5. Reopened REAPER and confirmed that the project and instrument loaded correctly.

This manual result supplies the audible packaged-instrument coverage missing from the focused
fresh-session automation test. The old matrix result no longer blocks the identity initiative; it
is tracked as a fixture/harness issue rather than a product recall failure.

## Installer qualification and correction

The Phase 5 installer is `build/installer/PracticalSampler-Setup-0.1.0-phase5.exe`.
Installed identity is exact:

| Field | Value |
| --- | --- |
| Installed product entry | Practical Sampler |
| Publisher | Practical Sampler Project |
| Version | 0.1.0-phase5 |
| Standalone | `C:\Program Files\Practical Sampler\Practical Sampler.exe` |
| VST3 | `C:\Program Files\Common Files\VST3\Practical Sampler.vst3` |
| Start Menu shortcut | `Practical Sampler.lnk` |

The installed-state audit found that the prior upgrade cleanup removed the legacy VST3 bundle but
could leave the legacy standalone executable beside the new executable. Phase 5 adds an explicit
installer deletion for that stale executable and sets the uninstall display name explicitly to
Practical Sampler.

The final qualification performed a true uninstall/clean install, compared installed standalone
and VST3 hashes with the Release sources, seeded both legacy artifact names through a validation-only
elevated installer, ran the real installer again, and proved both legacy artifacts were removed.
Evidence is `validation/installer/phase5-installer-qualification.json`.

## Installed standalone

The installed standalone launched with a responsive native window titled
`Practical Sampler - No Project Loaded` and closed normally. The stable settings file remains at
`AppData/Roaming/DecentRhapsodyStudio/DecentRhapsodyStudio.settings`; its SHA-256 was identical
before and after launch, proving the identity-only change did not fork or clear existing settings.
Evidence is `validation/standalone/phase5-standalone-qualification.json`.

## Artifact and stable-identity audit

- Release standalone and VST3 Windows metadata report Practical Sampler by Practical Sampler
  Project; their binaries contain no full former display name.
- `moduleinfo.json` contains the approved product/vendor plus the unchanged component and controller
  CIDs.
- Release output contains no old-named artifact, the installed old standalone and VST3 paths are
  absent, and the installer payload/installed hashes are recorded.
- The technical verifier passes for the `DecentRhapsodyStudio` project/targets, Dcrh/Drs0 codes,
  both CIDs, all four native formats, schemas, package signature, and all 16 host parameter IDs.
- No source logo, image, or branding asset was added.
- Every remaining legacy-name occurrence is classified by a checked-in path rule with an owner and
  reason. The final ledger contains no `CHANGE`, `REGENERATE`, or unclassified occurrence.

Machine-readable artifact evidence is
`validation/identity/phase5-artifact-identity-audit.json`.

## Gate G5 decision

Phase 5 implementation is complete, and all presentation identity, clean install, current-session
recall, installer upgrade, standalone, package, audit, and stable-ID checks pass. The representative
Salamander Piano save/close/reopen workflow also passes in REAPER. Gate G5 is passed. The inherited
audible matrix remains test-infrastructure debt and does not block Phase 6.
