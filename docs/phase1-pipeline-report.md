# Phase 1 Pipeline Report

This note captures the Sprint 2 automation slice that turns separate importer, loader, compile, and corruption checks into one CI-facing report.

## Current scope

`drs_phase1_pipeline_report` produces a single JSON artifact for the tiny open reference corpus and fails if any section regresses.

The report currently covers:

- loader status for the checked-in reference project and instrument
- importer status for each reference sample source
- compile-path status for deterministic generation, golden-file parity, and temp-directory reload
- corruption checks for known negative fixtures plus generated-artifact tampering

## Output

When run through CTest, the executable writes:

- `phase1-pipeline-report.json`

in the active test build directory.

The JSON structure is intentionally simple and reviewable in CI logs. It includes a top-level `passed` flag plus per-section pass states so the team can see whether a regression came from:

- loader behavior
- importer policy
- compile determinism
- corruption handling

## Why this slice exists

By the end of Sprint 2, the project needs more than a collection of good individual tests. It needs one report that answers: “Is the reference corpus healthy from source import through compiled-artifact validation?”

That single report is the bridge to Sprint 3, where streaming failures will be much easier to triage if import, compile, and corruption handling already have one trusted status artifact.
