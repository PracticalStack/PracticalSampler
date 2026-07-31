# Phase 2 Group Mixer Workflow

This release makes creator-authored group controls a supported workflow for mic positions, layered timbres, and ambient or mechanical noise such as pedal noise.

## Supported first-release scope

- Published player controls are authored macros.
- The supported lane type for the first release is group-scoped gain.
- Group pan is intentionally deferred until a later release.

## Creator workflow

1. Create or select groups that represent the controllable sources you want the player to mix.
   Examples: `close`, `room`, `felt`, `pad-layer`, `pedal-noise`.
2. In Routing, choose the target group scope and make sure that group owns one routing bus.
3. Add a curated gain insert to that group bus.
4. In Macros, create a macro and assign it to the gain parameter for that bus.
5. Turn on `Expose In Perform` for controls the player should see.
6. Publish the draft.
7. Verify the Perform surface shows the exposed controls in authored order and keeps helper macros hidden.

## Recommended examples

- Two-layer blend: `core` and `pad` groups, each with one exposed gain macro.
- Four-mic mixer: `close`, `player`, `room`, and `far` groups, each with one exposed gain macro.
- Pedal-noise lane: one hidden helper macro for internal shaping plus one exposed `Pedal Noise` gain macro for the player.

## Rules that still require Publish

- Group membership
- Routing-bus ownership
- DSP insert topology
- Macro target identity

Only published macro values move live at performance time.
