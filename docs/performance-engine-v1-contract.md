# Performance engine v1 contract

Status: accepted for implementation  
Decision date: August 2, 2026  
Scope: declarative-events Sprint 0; normative for Sprints 1-12

Continuous damper extension: the accepted HP-01 contract in
`continuous-damper-hp01-contract.md` supersedes this document's Boolean-only CC64
boundary only for routes that explicitly opt into continuous release control.
Legacy content retains the v1 behavior below.

## Boundary and ownership

The performance engine is a closed declarative event router. It compiles stable
authoring IDs and a fixed event vocabulary into immutable numeric tables. It is not
a scripting, expression, or modulation language. Publication rejects a project it
cannot flatten into the budgets below; the audio callback never reads author JSON,
looks up strings, allocates, locks, or dynamically grows event storage.

An immutable `SamplerRenderModel` owns compiled articulation descriptors,
event-indexed trigger-route ranges, choke masks, and RR-reset actions. Mutable
performance state belongs only to one playback lane: selected articulation, pedal
state, 128 held-note contexts, consumed-switch records, RR cursors, and diagnostics.
Preview and Performance never share that mutable state. A successful publication
installs a new immutable generation at a block boundary. Failed publication leaves
the last-known-good generation and its lane state active.

## Vocabulary

### Semantic events

| Event | Raw source | Required context | Meaning |
|---|---|---|---|
| `note-on` | Note-on with velocity greater than zero | channel placeholder, note, attack velocity, sample offset | A physical key press. It may select an articulation and be consumed, or route playable note-on zones. |
| `note-off` | Note-off, or note-on with velocity zero | channel placeholder, note, release velocity, retained attack context, sample offset | A physical key release. It may route key-up/mechanical samples even when the sounding note remains deferred by sustain. |
| `release` | A held note is permitted to enter release | retained attack context and pedal cause | The effective release of a sounding note. Damper/release samples normally use this event. |
| `pedal-down` | CC64 crosses from below 64 to 64 or greater | channel placeholder, CC value, sample offset | Sustain becomes active. Repeated CC64 values while already down produce no event. |
| `pedal-up` | CC64 crosses from 64 or greater to below 64 | channel placeholder, CC value, sample offset, deferred-note set | Sustain becomes inactive. Repeated CC64 values while already up produce no event. |

V1 routing is omni-channel, but all event and held-note records retain a typed
channel field. That field is not a promise of MPE routing in v1.

### Conditions, pitch, and playback

The only sustain conditions are `any`, `pedal-up`, and `pedal-down`. A condition is
evaluated against the lane pedal state at the semantic event's emission point. Thus a
physical `note-off` while the pedal is held matches `pedal-down`; the `release`
emitted by a subsequent pedal-up matches `pedal-up`.

The only pitch sources are `event-note` (the retained/event MIDI note) and
`fixed-root` (the route root key). A route is either `gated` or `one-shot`. A gated
route follows the normal voice release lifecycle. A one-shot starts once, ignores the
matching note-off for completion, and runs to sample completion unless it is choked or
stolen under the existing bounded voice policy. One-shot is not a request for an
unbounded voice reservation.

### Articulation activation and consumption

V1 articulation activation is latch-only: a `note-on` rule with one numeric MIDI
note selects a named articulation. Momentary, CC, program-change, and arbitrary
expression activation are outside v1. A matching latch rule must consume its note-on
before playable-zone routing and records the note in the lane-local consumed-note
bitset. The matching note-off is consumed as well, so a reserved switch can never
start or release a playable sample voice. A switch remains consumed even when it
selects the articulation already selected. A non-matching note is an ordinary
playable input.

## Same-offset ordering

For all raw MIDI messages at one sample offset, the engine observes this order:

1. Apply panic/reset and the activation-boundary policy.
2. Normalize raw MIDI and update transition state.
3. Evaluate consume/select-articulation actions.
4. Apply authored RR reset actions.
5. Apply choke actions to existing voices.
6. Release or defer existing note voices and emit effective `release` events.
7. Resolve and start trigger routes from the final state.

Within step 7, physical `note-off` routes launch before their same-offset effective
`release` routes; pedal transition routes launch after deferred release routes. In
particular, pedal-up is deterministic: **pedal-up transition -> deferred effective
release -> release-sample routes -> pedal-up-noise routes**. This is why a pedal-up
route must not be treated as a substitute for a release route.

RR resets run before route selection for that same event. Chokes act on existing
voices before any new voice starts. An event that would emit more than the action
budget is dropped atomically, with a bounded diagnostic increment; partial fan-out is
forbidden.

## Attack-context ownership

At `note-on`, the playback lane owns one fixed record per held MIDI note containing:

- MIDI note and channel placeholder;
- attack velocity and later physical release velocity;
- articulation index/ID at attack, not the currently selected articulation;
- activation generation/model reference needed by later release or choke work;
- physical-key-released, deferred-by-pedal, and effective-release-emitted flags.

The record is cleared only after the effective-release decision has been made and
the engine has retained every required originating-generation reference. A new
note-on for an occupied v1 note replaces the record deterministically and increments
the documented diagnostic. No release, choke, or route action may dereference a
retired published generation.

## Schema and migration policy

The project schema target is `drs.project` version 6 with `drs.authoring` version 5;
the compiled-instrument schema target is version 3. These versions are not emitted in
Sprint 0; they reserve the persisted contract for the following implementation
sprints.

For a legacy project with implicit articulation IDs inferred from zones, migration
creates deterministic first-class articulation definitions in first-zone appearance
order. Empty or absent zone articulation IDs map to the generated stable ID
`legacy-default`. The selected-zone articulation becomes default when available;
otherwise the first generated articulation is default. Legacy zones retain their
resolved IDs. Migration is explicit, deterministic, round-trippable, and undoable;
loading a legacy project alone does not silently change its serialized document.

Existing `performanceBanks.triggerSlots` are retained verbatim as phrase-bank or
legacy UI data. They neither create activation rules nor consume MIDI. A later phrase
bank may reference a first-class articulation, but it never owns the executable
articulation activation model.

## Fixed v1 limits

| Resource | Limit | Failure behavior |
|---|---:|---|
| Articulations | 64 | Publish error |
| Latch activation rules | 128 | Publish error |
| Compiled trigger routes | 4096 | Publish error with zone path |
| Choke groups / targets per route | 64 / 8 | Publish error |
| RR pools | existing 256 | Preserve existing rejection/diagnostic policy |
| Host MIDI events per block | existing 128 | Count and drop excess; never allocate |
| Semantic dispatch records per block | 512 | Count and drop excess; never allocate |
| Actions from one semantic event | 64 | Drop the event atomically and count it |
| Held-note contexts per lane | 128 | Deterministic replacement and diagnostic |

Measurement baseline: the committed two-articulation strings fixture has two
playable routes and two latch rules; the Piano Lite timeline has five trigger routes.
The checked-in jRhodes SFZ characterization has 225 regions, while the permanent SM
Drums corpus has 3,358 regions. The latter stays within the 4,096 route ceiling with
738 slots of headroom. These are compile-time capacity checks, not permission to
allocate beyond a limit. A production Piano Lite package is not currently present in
the repository; its committed event timeline is the Sprint 0 semantic fixture and
must be replaced by a measured authored package before the Sprint 12 host gate.

## Fixtures and future-test mapping

`content/runtime/performance-engine/sprint0/two-articulation-strings.fixture.json`
proves C0/MIDI 12 selects Sustain and D0/MIDI 14 selects Staccato, with neither
switch starting a sample voice. `piano-lite-event-timeline.json` freezes physical
note-off, sustained deferred release, pedal-up, and pedal-down ordering.

The direct-only `drs_performance_engine_s0_red_tests` executable names the missing
production seams. It is intentionally red during Sprint 0 and is not a CTest test or
an `drs_all_tests` dependency. Each later sprint promotes its completed seam to a
registered green behavioral test; deleting or treating the expected-red exit as a
passing release test is prohibited.
