# Phase 1 Pipeline Report

This note captures the Sprint 2 automation slice that turns separate importer, loader, compile, and corruption checks into one CI-facing report.

## Current scope

`drs_phase1_pipeline_report` produces a single JSON artifact for the tiny open reference corpus and fails if any section regresses.

The report currently covers:

- loader status for the checked-in reference project and instrument
- stream-reader status for the checked-in reference `.drstrm` artifact
- stream-scheduler simulation status for background page requests against the reference container
- voice-runtime simulation status for allocation, wait, resume, and cleanup against the reference instrument
- note-routing status for default-articulation selection plus low/high velocity routing through the reference instrument
- load-profile status for named profile discovery, per-voice prefetch clamping, live downgrade behavior, and dormant-page purge
- runtime-counter status for page misses, head usage, read latency, voice count, and purge activity during stress and idle recovery
- importer status for each reference sample source
- compile-path status for deterministic generation, golden-file parity, and temp-directory reload
- corruption checks for known negative fixtures plus generated-artifact tampering
- state-recall status for standalone export/reload, plugin host-state export/reload, and invalid restore preservation
- macro-state compare status for runtime macro values versus host-facing `macro.*` plugin parameters
- error-handling status for missing-pack/content, checksum, schema, and partial-artifact probes
- a nightly validation summary block with separate pass/fail signals for `load`, `play`, `stateRecall`, and `errorHandling`

## Output

When run through CTest, the executable writes:

- `phase1-pipeline-report.json`

in the active test build directory.

The JSON structure is intentionally simple and reviewable in CI logs. It includes a top-level `passed` flag, per-section pass states, and a `nightlyValidation` summary so the team can see whether a regression came from:

- loader behavior
- playback/runtime behavior
- state recall or macro bridging
- error handling
- importer policy
- compile determinism
- corruption handling

## Why this slice exists

By the end of Sprint 2, the project needs more than a collection of good individual tests. It needs one report that answers: “Is the reference corpus healthy from source import through compiled-artifact validation?”

That single report is the bridge to Sprint 3, where streaming failures will be much easier to triage if import, compile, and corruption handling already have one trusted status artifact.

With the load-profile slice in place, the same report now also answers whether the reference corpus can shift between `performance` and `eco` budgets without silently invalidating active playback.

With the note-routing slice in place, it also records which zones the reference instrument selected for low/high default and lead triggers so routing regressions are visible before a larger playback corpus lands.

With the runtime-counter slice in place, the same report now also captures one benchmark-style observability pass so the team can see whether misses, head usage, read latency, voice count, and purge activity are all moving in the expected direction.

With the Sprint 4 regression-automation slice in place, the same report now also answers whether:

- save and reload still work through the real standalone and plugin shell seams
- the host-facing macro surface still matches the restored runtime macro state
- missing-pack/content and other planned failure paths still fail gracefully without discarding the last known-good session
