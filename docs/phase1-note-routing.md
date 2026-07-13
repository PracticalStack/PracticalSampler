# Phase 1 Note Routing

This note captures the fifth Sprint 3 slice: the first product-owned note-trigger path for articulation and velocity selection.

## Current scope

Phase 1 note routing now supports:

- default-articulation selection when a note trigger does not name an articulation explicitly
- explicit articulation selection for the first realistic `lead` path in the reference instrument
- velocity-layer selection inside each articulation
- deterministic zone selection that can flow directly into the existing voice and streaming runtime

The current reference instrument stays intentionally small, but it is no longer just a pair of hand-picked zone ids. It now exposes the first trigger-driven route that can choose between base and accent zones for the same articulation.

## Reference fixture shape

The tiny open instrument now carries:

- one default `sustain` articulation with base and accent velocity layers
- one explicit `lead` articulation with base and accent velocity layers
- stable zone ids that make the selected route visible in tests and CI artifacts

That is enough to prove the routing seam without needing a much larger private content set yet.

## Validation

`drs_phase1_note_routing_tests` now proves:

- default note triggers resolve into the default articulation
- low and high velocities select different sustain zones
- explicit `lead` triggers resolve into the lead articulation
- low and high velocities select different lead zones
- unknown articulations and unmapped note ranges fail loudly
- routed note triggers still enter the existing streaming voice path and resume after page reads

The shared `drs_phase1_pipeline_report` artifact now also includes a `noteRouting` section so CI records which zones the reference instrument selected for low/high default and lead triggers.
