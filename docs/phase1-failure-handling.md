# Phase 1 Failure Handling

This note captures Sprint 4 task `P1-404`: product-owned handling for obvious invalid-content and partial-artifact failures.

## Current scope

The shell now exposes explicit content-failure probes for:

- missing content
- bad checksums
- schema mismatch
- partially compiled artifacts

Each probe is designed to fail loudly but safely:

- it returns an actionable state string
- it records detailed issues
- it leaves the last known-good preset session intact
- it updates the diagnostics panel so the failure is visible instead of silent

## Checked-in fixtures

Phase 1 now carries product-owned negative fixtures for:

- `negative-corpus/missing-sample-file`
- `negative-corpus/schema-mismatch`
- `negative-corpus/partial-compiled-artifact`

The bad-checksum path is generated from the checked-in reference stream container into a temp location so the probe exercises the exact runtime checksum-validation seam.

## Manual QA script

1. Open the standalone shell or plugin editor.
2. Load the lead fixture so the shell is on a known-good `performance` / `lead` session.
3. Run each content probe button:
   - `Probe Missing`
   - `Probe Checksum`
   - `Probe Schema`
   - `Probe Partial`
4. After each probe, confirm:
   - the diagnostics failure state becomes non-empty
   - the issue text names the specific failure category
   - the load profile and articulation remain on the last known-good session
5. Click `Clear Probe` and confirm the visible failure state clears.

## Validation

`drs_phase1_failure_handling_tests` now proves that:

- each planned failure category reports a specific actionable failure
- each category fails gracefully instead of succeeding partially or crashing
- the engine preserves the last known-good `performance` / `lead` session after every failed probe
- clearing the probe removes the visible failure state once no restore error remains
