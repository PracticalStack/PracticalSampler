# HP-03 — Continuous controller state and dynamic release

Status: complete on August 15, 2026.

HP-03 implements the realtime half-pedal release path. Repedaling lifecycle
hardening and release-trigger uniqueness remain owned by HP-04. Pedal noise,
sympathetic resonance, and physical string coupling remain out of scope.

## Controller and sustain behavior

- Continuous-damper programs retain exact 7-bit controller values through host
  normalization, performance-lane evaluation, voice-pool routing, reset, and
  activation. Activation preserves the live table; initial preparation and reset
  reapply compiled controller defaults.
- Legacy programs retain their existing reset-on-activation behavior.
- The authored sustain controller and threshold produce Boolean pedal edges.
  Intermediate or repeated values remain visible to release control without
  duplicating pedal-down or pedal-up mechanics.
- Sustain reassignment is independent of the dynamic-release controller: for the
  focused Salamander projection, CC64 controls release duration even when another
  controller owns note deferral.

## Dynamic release law

Each voice begins release from the immutable damper declaration captured by its
route and activation generation:

`seconds = clamp(baseReleaseSeconds + amount * curve[controllerValue], 0.001, 100)`

The authored release shape remains unchanged. A relevant controller change on an
already-releasing voice captures its current envelope level, replaces only the
future segment duration, and renders the first updated sample from that exact level.
Repeated exact values are no-ops. The update path is bounded to the fixed 24-voice
pool, uses stack state only, and performs no callback allocation.

Older voices continue to reference their original render model, so a controller
event after activation cutover evaluates an old voice with its old curve and a new
voice with the newly active curve.

## Evidence

The registered `drs.continuous_damper.hp03` test covers:

- exact CC64 value 62 preservation without a binary pedal transition;
- authored CC90 sustain edges and repeated-value de-duplication;
- controller-table preservation across continuous-model activation;
- the duration formula at physical note-off;
- sample-positioned release updates with exact envelope-level continuity;
- no restart on a repeated exact controller value;
- strict tail ordering at controller values 0, 20, 32, 42, 54, 62, 63, 64, and
  127, including an authored-only 63-to-64 delta;
- independent CC64 release control and CC90 note deferral; and
- generation-owned curve evaluation across activation cutover.

Compatibility coverage includes the voice kernel, binary pedal state machine, and
voice-generation cutover suites. The HP-01 direct harness now reports
`preserve-continuous-cc64`, `dynamic-release-envelope`,
`sustain-controller-reassignment`, and `generation-owned-damper-update` as promoted
green behavior. `repedal-still-audible` and `release-trigger-uniqueness` remain red
until HP-04.
