# Phase 1 Reference Macro Map

This note captures Sprint 5 task `P1-502`: binding the Phase 1 reference macros to stable runtime controls and documenting their intended sound impact.

## Stable ownership

The reference instrument now treats the two authored macros as owned runtime bindings:

- `tone`
  - owner: `preview.triggerVelocity`
  - intent: move the reference instrument from softer attacks into accent territory
  - current Phase 1 binding: converts the macro value into the preview trigger velocity used by the runtime seam

- `motion`
  - owner: `preview.noteTravel`
  - intent: add movement by shifting the played note across the reference zone range
  - current Phase 1 binding: converts the macro value into a `-12` to `+12` semitone preview-note offset

## Why these bindings

Phase 1 still uses a compact runtime proof, not a finished synth voice. The goal of this slice is to give each macro one clear, stable job that is:

- visible on the Sprint 5 performance surface
- preserved through the Sprint 4 host/state seam
- meaningful enough that future audio binding work does not need to reinterpret ownership later

`tone` owns brightness/accent bias.

`motion` owns note travel.

Those are narrow but contributor-friendly anchors for the reference instrument.

## Surface behavior

The performance surface now reflects both the raw macro value and the active effect text:

- `tone` reports `Soft attack`, `Balanced attack`, or `Accent attack`
- `motion` reports the active semitone offset, such as `+6 st` or `-4 st`

The preview line also shows the applied macro summary plus the effective note and velocity used by the runtime seam.

## Validation

`drs_phase1_macro_bridge_tests` and `drs_phase1_state_recall_tests` now prove that:

- the macro ownership keys remain stable
- the visible effect text changes when macro values change
- preview playback responds to the macro bindings through routed zone selection and effective note/velocity changes
- the same macro effects are still present after save/reload in both standalone and plugin shells
