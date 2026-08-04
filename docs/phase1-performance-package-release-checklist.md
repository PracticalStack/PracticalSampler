# Phase 1 Performance Package Release Checklist

Prepared on August 4, 2026 for the playable package cutover.

## Pre-release gate

- Confirm `drs.phase1.fixture_tool_verify` passes so the checked-in package corpus and baseline snapshots are current.
- Confirm `drs.phase1.performance_package` passes so writer determinism and package structure checks remain green.
- Confirm `drs.phase1.performance_package_loader` passes so corruption and version-skew diagnostics remain category-stable.
- Confirm `drs.phase1.performance_package_session` passes so exported packages reopen in both shells with performance-only semantics.
- Confirm `drs.phase1.performance_package_host_validation` passes so package-backed playback remains audible in standalone and plugin paths.
- Confirm `drs.phase1.performance_package_release_gate` passes and writes `phase1-performance-package-release-gate.json`.

## Customer smoke gate

- Export at least one reviewed customer-ready playable package from an editable authoring project.
- Open that exported package in standalone and confirm:
  - only the Perform view is visible
  - no authoring save prompt appears
  - playback works from the performance keyboard
- Open that exported package in the plugin shell and confirm the same behavior.
- Reopen the same package after a fresh app/plugin launch and confirm the performance-only session still holds.

## Regression gate

- Verify the checked-in package corpus still includes:
  - `valid.drpkg`
  - `truncated.drpkg`
  - `tampered.drpkg`
  - `wrong-version.drpkg`
  - `missing-payload.drpkg`
  - `checksum-mismatch.drpkg`
- Review `phase1-performance-package-release-gate.json` for:
  - deterministic package bytes
  - successful exported-package reopen
  - performance-only UX invariants
  - explicit failure-category coverage

## Sign-off

- Product confirms that editable-project save flows and playable-package export flows are described as different actions in release notes and support docs.
- QA confirms the operator guide and compatibility policy docs match the shipped behavior.
- Support confirms the failure-category vocabulary is available for triage.
