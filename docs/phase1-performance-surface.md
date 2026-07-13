# Phase 1 Performance Surface

This note captures Sprint 5 task `P1-501`: the first compact playback-oriented UI for the Phase 1 milestone.

## Current scope

Both the standalone shell and the plugin editor now open on the same shared performance surface instead of a diagnostics-first screen.

The surface currently includes:

- a compact header with the loaded reference instrument and load-state badge
- explicit `Load Default` and `Load Lead Demo` entry points
- articulation buttons for the authored reference instrument
- a macro strip driven by the same runtime/session state and host parameter seam from Sprint 4
- an on-surface keyboard that drives preview playback and the plugin shell's product-owned render seam
- a `Show Diagnostics` affordance that reveals the existing developer-facing diagnostics panel without making it the primary UI

With the macro-map slice in place, the same surface now also shows each macro's current effect:

- `tone` reports the current attack bias
- `motion` reports the current semitone travel offset

## Why this slice exists

Sprint 4 proved the engine seams, preset recall, diagnostics, and host parameter path.

Sprint 5 starts converting those seams into something a contributor can actually use without hunting for hidden debug actions. The Phase 1 milestone now has one obvious entry point where someone can:

- load the default or lead reference state
- switch articulations
- move macros
- play notes
- open diagnostics only when they need deeper troubleshooting

## Manual QA

1. Open the standalone shell or plugin editor.
2. Confirm the load badge reports the reference instrument as ready.
3. Click `Load Lead Demo` and verify the patch status line switches to the `performance` load profile and `lead` articulation.
4. Play the on-screen keyboard and confirm the preview line reports the triggered note and routed zone.
5. In the plugin shell, confirm the same on-screen keyboard produces audible output without requiring external host MIDI input.
6. Confirm the keyboard range follows the current playable window for the selected articulation and macro state.
7. Switch articulations and play again to confirm the routed zone changes with the selected articulation.
8. Move the macro sliders and verify the values update visibly.
9. Open diagnostics with `Show Diagnostics`, confirm the detailed panel appears, then hide it again.

## Validation

`drs_phase0_smoke_tests` now proves that:

- both shells instantiate the new performance surface as their root UI
- the surface exposes the Sprint 5 keyboard and diagnostics affordance
- the engine preview-play seam can select the lead articulation and resolve a playable preview note cleanly
- the plugin processor accepts the performance-surface keyboard notes and renders audible output without host MIDI input
