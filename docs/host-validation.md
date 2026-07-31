# Host Validation

This document records the current Windows host-load workflow for the Phase 0 VST3 shell.

## Current expectation

The build produces a valid VST3 bundle under the build tree, but it does not auto-install it into the system VST3 location.

That means a DAW such as Reaper will only discover the plugin if one of these is true:

- the full `.vst3` bundle is copied into `C:\Program Files\Common Files\VST3\`
- or Reaper is configured to scan the build artefact directory that contains the `.vst3` bundle

Pointing Reaper at the inner `Contents\x86_64-win\` folder is not sufficient. The scan target must be the directory that contains `Decent Rhapsody Studio.vst3`.

## Recommended local workflow

1. Build and test the plugin:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\bootstrap-windows.ps1 -RunTests
```

2. Install the built bundle into the standard Windows VST3 directory:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\install-vst3-windows.ps1
```

3. In Reaper, force a rescan of VST plugins.

4. Look for `Decent Rhapsody Studio` under instruments, since the current shell is built as a synth.

## Alternate workflow for a non-installed build

If you do not want to copy into `C:\Program Files\Common Files\VST3\`, add this directory to Reaper's VST scan paths:

```text
<repo>\build\vs2022-debug\app\drs_plugin_bundle_artefacts\Debug\VST3
```

or for release builds:

```text
<repo>\build\vs2022-release\app\drs_plugin_bundle_artefacts\Release\VST3
```

Again, the path should be the folder that contains the `.vst3` bundle, not the folder inside the bundle.

## Current validation signal

The Phase 0 smoke test now does all of the following:

- verifies the plugin bundle exists after build
- verifies the bundle can be scanned as VST3 through JUCE's host layer
- verifies the plugin description resolves as `Decent Rhapsody Studio`

If Reaper still does not discover the plugin after installation or scan-path correction, the next useful artifact is Reaper's scan result for this bundle.

## DAW project-recall validation

Project-aware recall is documented in [daw-host-state-recall.md](daw-host-state-recall.md).
The checked host matrix and signed artifacts are recorded in
[host-state-reaper-validation-evidence.md](host-state-reaper-validation-evidence.md).

For recall validation, do not stop after confirming that macro parameters return. Verify that the
reserialized VST3 chunk contains `drs.hostState`, the expected project ID and manifest digest, and
a complete published checkpoint after transport has exercised block-boundary activation.

The required matrix is:

- editor open and editor closed;
- moved project locator;
- matching project ID with a changed manifest digest;
- missing sample/content preparation failure; and
- duplicated instances restored independently.

Missing, moved, changed, and invalid content must preserve the safe startup state and expose
non-modal recovery. They must never present the reference instrument as the recalled project.

## WAV import startup validation

WAV startup/import host validation is recorded in
[wav-import-host-validation-evidence.md](wav-import-host-validation-evidence.md).
The release-facing closure evidence for retiring the old synchronous shell path is recorded in
[wav-import-release-evidence.md](wav-import-release-evidence.md).

The required WAV-705 matrix is:

- standalone construction plus project replace with missing-local sample paths;
- standalone construction plus project replace with removable-drive-like sample paths;
- standalone construction plus project replace with UNC-like network sample paths; and
- REAPER startup for the same three project variants with signed evidence for instantiation timing,
  parameter visibility, and restored track-chunk capture.

All six cases must preserve zero startup sample I/O on the standalone seam, remain `not-run` until
explicit import/validation work is requested, and keep the REAPER VST3 instance online with the
safe startup macro values visible immediately after instantiation.

## Sprint 4 diagnostics spot-check

Once the plugin or standalone shell opens, the Status panel should now also expose a developer diagnostics block.

The minimum Sprint 4 spot-check is:

- verify the panel shows a load profile, cache budget, and non-zero page-miss count
- verify `Load Lead Fixture` switches the panel to the performance profile
- verify `Inject Invalid State` surfaces a visible failure state without silently overwriting the prior valid session
- verify `Probe Missing`, `Probe Checksum`, `Probe Schema`, and `Probe Partial` each surface a visible content failure while preserving the prior valid session
- verify moving the macro sliders changes the visible macro values and, in a host, appears as `macro.*` automation parameters

## Sprint 5 performance-surface spot-check

Once the shell opens, the first screen should now be the compact performance surface instead of the full diagnostics view.

The minimum Sprint 5 spot-check is:

- verify the load badge reports the reference instrument as ready
- verify `Load Default` and `Load Lead Demo` update the visible patch status line
- verify the articulation buttons switch the selected articulation shown on the surface
- verify playing the on-screen keyboard updates the preview status with the note and routed zone
- verify the macro strip shows both the raw value and the current effect text
- verify higher `tone` values push preview playback toward accent behavior while `motion` changes the effective previewed note
- verify `Show Diagnostics` reveals the deeper Sprint 4 diagnostics panel without leaving the performance surface
