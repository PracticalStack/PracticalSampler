# Phase 1 Performance Package Validation

This note captures the Sprint 7 package hardening workflow for the tiny open instrument sealed package corpus.

## Checked-in corpus

The checked-in package fixtures live at:

- `content/runtime/phase1/reference-corpus/tiny-open-instrument/performance-package-corpus/`

The corpus currently includes:

- `valid.drpkg`
- `truncated.drpkg`
- `tampered.drpkg`
- `wrong-version.drpkg`
- `missing-payload.drpkg`
- `checksum-mismatch.drpkg`
- `index.json`

`index.json` records the expected failure category and signature issue text for each negative fixture.

## Verify

Use the contributor wrapper:

- `powershell -ExecutionPolicy Bypass -File .\tools\package-phase1-reference-instrument.ps1 -Mode Verify`

That path:

- builds `drs_phase1_runtime_fixture_tool`
- regenerates the package corpus in a temp location
- compares the generated corpus against the checked-in `.drpkg` fixtures and `index.json`

## Refresh

If the sealed package format changes intentionally, refresh the checked-in corpus with:

- `powershell -ExecutionPolicy Bypass -File .\tools\package-phase1-reference-instrument.ps1 -Mode Refresh`

That refresh updates both:

- `tiny-open-instrument/package-manifest.json`
- `tiny-open-instrument/performance-package-corpus/*`

## Focused validation

After a refresh, run:

- `ctest --preset test-debug -R "drs.phase1.fixture_tool_verify|drs.phase1.performance_package|drs.phase1.performance_package_loader|drs.phase1.performance_package_session|drs.phase1.performance_package_host_validation|drs.phase1.performance_package_release_gate" --output-on-failure`

The release-gate test writes:

- `phase1-performance-package-release-gate.json`

That artifact is the primary Sprint 8 go/no-go snapshot for package determinism, exported-package reopen, performance-only UX, and failure reporting.

## Triage

Use the failure category first:

- `package-format-failure`: header, version, cleartext metadata, or TOC structure no longer matches the reader contract.
- `decryption-failure`: authenticated data no longer matches the sealed TOC or payload bytes; check tampering, AAD drift, and crypto metadata.
- `payload-corruption`: the sealed package opens, but required payloads are missing or decrypted bytes fail size or checksum validation.
- `playback-compatibility-failure`: runtime payloads decrypt, but they no longer agree on instrument/runtime semantics needed for activation.

If `drs.phase1.fixture_tool_verify` fails:

- compare the changed fixture checksums in `performance-package-corpus/index.json`
- rerun the focused package tests to see whether the drift is writer-side, reader-side, or activation-side
- refresh the corpus only after confirming the format change is intentional

If `drs.phase1.performance_package_release_gate` fails:

- inspect the failing section in `phase1-performance-package-release-gate.json`
- treat `compatibilityPolicy` failures as contract blockers
- treat `determinism`, `reopenAndPerformanceOnlyUx`, or `failureReporting` failures as release blockers
