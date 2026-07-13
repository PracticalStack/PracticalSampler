# Phase 1 Macro Bridge

This note captures Sprint 4 task `P1-405`: the first product-owned macro bridge between runtime state, the in-app shell surface, and host-facing plugin parameters.

## Current scope

Phase 1 macros are now exposed through a shared shell bridge:

- `EngineFacade` owns authored macro descriptors plus current macro values
- the shared status panel renders one slider row per authored macro
- the standalone shell writes those slider changes directly into runtime session state
- the plugin shell maps those same macro ids to host-facing parameters (`macro.<id>`)

This keeps the macro seam dynamic instead of hard-coding special-case UI for `tone` and `motion`.

## Host-facing parameter rules

The VST3 shell now exposes one parameter per authored macro using:

- a stable parameter id format: `macro.<macroId>`
- the authored macro display name for the host-visible parameter name
- the authored min/max/default range from the reference instrument manifest

That gives later UI work a stable bridge to bind against without reimplementing parameter naming or range rules.

## Validation

`drs_phase1_macro_bridge_tests` now proves that:

- standalone macro edits update runtime macro state and survive a save/reload round-trip
- the plugin shell exposes host-facing `macro.tone` and `macro.motion` parameters
- host-style parameter updates propagate back into runtime macro state
- plugin save/reload preserves both the runtime macro state and the host-facing parameter values
