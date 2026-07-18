# Phase 1 Sprint 3 Prepared Playback Error Mapping

This note captures `S3.3-T4` from the Sprint 3 companion plan on July 18, 2026.

Goal of this step: normalize worker-side prepared-playback import failures so the caller receives structured findings for the main failure categories instead of one generic decode error.

## What changed

- `PreparedPlaybackService::prepare(...)` now classifies source-import failures before emitting a finding.
- the worker now distinguishes three structured error codes:
  - `prepared-sample-source-missing`
  - `prepared-sample-format-unsupported`
  - `prepared-sample-decode-failed`
- each finding preserves the importer state text in the message so callers still see the specific reason inside the broader category:
  - missing file
  - unsupported audio format
  - decode or policy rejection detail

## Why this matters

Sprint 3 now exposes worker-owned preparation failures as stable contract categories instead of requiring the shell or tests to parse free-form error strings.

That gives us a cleaner boundary for later queue, diagnostics, and shell-facing metrics work:

- category-level assertions can stay stable
- detailed importer context is still preserved for debugging

## Verification

- `tests/src/Phase1PreparedPlaybackTests.cpp`
  - accepted snapshot then missing file at prepare time yields `prepared-sample-source-missing`
  - accepted snapshot then rewritten non-audio file yields `prepared-sample-format-unsupported`
  - accepted snapshot then policy-rejected 96 kHz WAV yields `prepared-sample-decode-failed`
- `tests/src/Phase1PreparedPlaybackWorkerTests.cpp`
  - unchanged queue/worker success path remains green around the new mapping
- `tests/src/Phase1DraftPlaybackFacadeTests.cpp`
  - facade-driven preview/publish orchestration remains green after the worker finding changes

Validated with:

- `drs_phase1_prepared_playback_tests`
- `drs_phase1_prepared_playback_worker_tests`
- `drs_phase1_draft_playback_facade_tests`
