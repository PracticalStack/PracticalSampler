# Phase 1 Performance Package Operator Guide

Prepared on August 4, 2026 for support and internal QA.

## What a playable package is

A playable package is a sealed `.drpkg` file that opens in Decent Rhapsody Studio as a performance-only session.

It is intentionally different from an editable project:

- editable project: `.drsproj`, authoring available, save/edit/import workflows
- playable package: `.drpkg`, no authoring tab, no project save flow, performance-only playback

## Expected user-visible behavior

When a package is opened successfully:

- the workspace shows only the Perform view
- authoring controls such as the map/zone editor are absent
- host state exports do not include project bindings
- playback works through the same runtime engine used by exported projects

## Triage flow

1. Ask whether the user opened a `.drsproj` or a `.drpkg`.
2. If it is a `.drpkg`, collect the visible error text and classify it by failure category.
3. Confirm whether the failure happens on open, on reopen, or only on first playback.
4. If the package opens but playback fails, collect the package file and the release-gate artifact version used by the build.

## Failure categories

### `package-format-failure`

Use when the package header or cleartext metadata is not readable by the current build.

Common examples:

- truncated package
- wrong schema or unsupported format version
- package requires a newer reader schema version

### `decryption-failure`

Use when the package structure is present but authenticated package data no longer opens.

Common examples:

- tampered TOC
- payload authentication mismatch
- packaging bug that changed authenticated-data inputs

### `payload-corruption`

Use when the package opens far enough to identify payloads, but required payloads are missing or damaged.

Common examples:

- missing payload records
- checksum mismatch
- decrypted payload size mismatch
- malformed runtime payload JSON

### `playback-compatibility-failure`

Use when payloads load but do not agree on runtime semantics required for activation.

Common examples:

- manifest/runtime instrument mismatch
- runtime stream and instrument identity mismatch
- activation snapshot cannot become playable

## Operator commands

- Verify corpus and package docs:
  - `powershell -ExecutionPolicy Bypass -File .\tools\package-phase1-reference-instrument.ps1 -Mode Verify`
- Refresh package docs and corpus after an intentional format change:
  - `powershell -ExecutionPolicy Bypass -File .\tools\package-phase1-reference-instrument.ps1 -Mode Refresh`
- Run the focused package validation slice:
  - `ctest --preset test-debug -R "drs.phase1.performance_package|drs.phase1.performance_package_loader|drs.phase1.performance_package_session|drs.phase1.performance_package_host_validation|drs.phase1.performance_package_release_gate" --output-on-failure`

## Escalation

Escalate immediately if:

- a `.drpkg` session exposes authoring controls
- a package session resolves raw sample files outside the sealed payload
- Save Project appears as the recovery path for a package session
- the release-gate artifact fails determinism or exported-package reopen checks
