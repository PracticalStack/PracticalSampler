# HP-04 — Repedal catch and lifecycle hardening

Status: complete on August 15, 2026.

HP-04 hardens HP-03's continuous release update into a complete repedaling
lifecycle. Pedal noise, sympathetic resonance, and physical string coupling remain
out of scope.

## Catch semantics

- A rising release-controller value on a still-audible releasing voice is recorded
  as a repedal catch. The voice remains in the release lifecycle; it does not return
  to attack or key-held sustain.
- The update retains voice ID, trigger ID, source route, articulation-at-attack,
  activation generation, authored curve, and the exact current envelope level.
- Lost energy is never restored. The new duration applies only to the future
  segment, and multiple controller changes at one sample offset resolve in stable
  input order before that sample renders.
- Finished and stolen voices are not scanned as eligible release voices and cannot
  be revived.

The fixed 24-slot scan remains allocation-free. Per-block and cumulative playback
diagnostics now distinguish all dynamic release updates from upward repedal catches.
Voices also retain the trigger identity that the pool already assigned to each
physical or semantic trigger.

## Release uniqueness and teardown

Repedal controller changes do not pass through the note-off or effective-release
semantic paths. A physical key-up therefore emits one effective release and starts
at most one authored release-trigger sample, regardless of later catch count.
Sequential repeated pitches receive one such pair per gesture.

All-notes-off enters the ordinary release lifecycle and its still-audible tail may
be caught. All-sound-off/reset, transport-discontinuity reset, finished playback,
voice stealing, and package close terminate or replace ownership immediately; later
controller events cannot recreate those voices. A transport reset reapplies the
active program's exact controller defaults and continuous-damper configuration, so
new host CC64 gestures work immediately without waiting for another activation.
Activation replacement remains the
intentional exception: an audible retired-generation tail remains catchable through
its immutable originating route until it finishes or is explicitly terminated.

Preview and Performance retain independent controller tables, voice pools, and
repedal diagnostics.

## Evidence

The registered `drs.continuous_damper.hp04` test covers:

- the 127-to-32-to-110 catch timeline with exact envelope continuity;
- a same-offset 0/127/20/110 staircase with no callback allocation;
- unchanged voice, trigger, route, and activation identities across catches;
- one physical note-off, one effective release, and one actual release-trigger
  sample per sequential repeated-pitch gesture;
- deterministic stealing of the oldest release and no catch of the stolen ID;
- no revival after natural finish, panic/reset, transport reset, or package close,
  plus immediate host-CC64 recovery after transport reset;
- all-notes-off release and catch behavior;
- old-generation catch after activation replacement; and
- Preview/Performance isolation plus cumulative update/catch diagnostics.

The HP-01 direct harness now reports all seven named seams as promoted green.
`repedal-still-audible` and `release-trigger-uniqueness` name
`drs.continuous_damper.hp04` as their registered owner.
