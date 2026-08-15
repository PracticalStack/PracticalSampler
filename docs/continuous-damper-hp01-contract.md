# Continuous Damper HP-01 Contract

Status: accepted for implementation on 2026-08-15  
Scope: HP-01 contract, deterministic fixtures, and direct-only expected-red seams  
Supersedes: the CC64 Boolean-only boundary in `performance-engine-v1-contract.md` when a route explicitly opts into continuous damper release control

## Outcome

Decent Rhapsody Studio will preserve MIDI CC64 values from 0 through 127 and use
them to alter the remaining amplitude-envelope release time of eligible sounding
voices. A later increase in CC64 can catch a still-audible release tail without
restoring energy already lost. Existing projects remain binary-sustain instruments
unless authored or imported content explicitly declares continuous release control.

This extension is a bounded piano-damper path. It is not a general modulation
matrix and it does not add pedal resonance, sympathetic resonance, string coupling,
or a physical piano model.

## Terms

| Term | Contract meaning |
| --- | --- |
| Physical key down | A note-on has not yet received its matching physical note-off. |
| Binary sustain | A declared controller and threshold decide whether physical note-off is deferred. |
| Damper position | The exact current 0–127 value of the release-control CC, normally CC64. |
| Effective release | The single transition after which a source note may emit its authored `release` event and release-trigger routes. |
| Dynamic release | An already-running release segment may change its future duration at a sample-positioned CC event. |
| Repedaling | Raising the damper controller while a voice is releasing lengthens the remaining tail from its current envelope level. |
| Catchable voice | A voice that is still allocated and audible in its release segment; a finished or stolen voice is not catchable. |

## Canonical constants

The compile-time reservations live in
`engine_adapter/include/drs/engine/ContinuousDamperContract.h`.

| Concern | Value |
| --- | ---: |
| Legacy native sustain controller | 64 |
| Legacy native sustain threshold | 64 |
| Imported SFZ default `sustain_cc` | 64 |
| Imported SFZ default `sustain_lo` | 0.5 |
| Continuous half-pedal release controller in this iteration | 64 |
| Compiled curve size | 128 normalized entries |
| Minimum effective release | 0.001 seconds |
| Maximum effective release | 100 seconds |
| Reserved project / authoring schema | 7 / 6 |
| Reserved runtime instrument schema | 5 |

HP-01 reserves these schema versions but does not emit them. HP-02 owns migration,
serialization, validation, compilation, and package round-trip behavior.

## Controller-state contract

1. Host normalization retains controller number, exact value, sample offset,
   channel placeholder, and stable input sequence.
2. Each playback lane owns its own 128-entry current-controller table. Preview and
   Performance never share mutable controller state.
3. Binary sustain state is derived from the declared sustain controller and
   threshold. It is not hard-coded to CC64 once authored configuration exists.
4. Pedal-down/up semantic events, pedal-mechanics routes, and pedal Round Robin
   resets remain edge-triggered. Intermediate controller changes do not duplicate
   them.
5. Continuous release control observes every exact CC64 value, including repeated
   values. A repeated value may be a no-op after bounded state comparison, but it
   must not be rewritten to 0 or 127.
6. Reset restores compiled controller defaults. All-sound-off/panic still ends
   voices immediately and is never catchable.

## Release control law

For the supported route path:

```
effectiveReleaseSeconds = clamp(
    baseReleaseSeconds + releaseControlAmountSeconds * curve[cc64Value],
    0.001,
    100.0)
```

- The curve has exactly 128 compiled normalized values.
- Explicit SFZ `v000` through `v127` points are retained.
- Gaps use deterministic linear interpolation between the nearest explicit points.
- Values before the first explicit point and after the last explicit point use the
  applicable SFZ/default endpoint policy frozen by HP-02.
- Non-finite points, duplicate curve indices, malformed `vNNN` names, invalid
  references, and out-of-range results produce stable import or publication
  findings outside audio.
- The focused runtime target is amplitude-envelope release controlled by CC64.
  Other `*_onccN` targets remain reported, not silently approximated.

## Dynamic release and repedaling

At physical note-off, a route not deferred by binary sustain enters effective
release exactly once. A route deferred by binary sustain enters effective release
exactly once when the declared sustain controller crosses to the up side.

With dynamic release enabled, a later CC64 value updates a catchable voice at the
event sample. The update must:

- preserve voice identity, route, articulation-at-attack, trigger identity, and
  activation generation;
- preserve the current envelope level, with no upward gain jump;
- alter only the future release trajectory;
- retain the authored release shape continuously;
- perform no allocation, lock, file I/O, string lookup, or unbounded search;
- avoid allocating a replacement voice or replaying attack material;
- avoid emitting another physical note-off, effective `release`, or release sample.

Repedaling does not return a voice to attack or ordinary key-held sustain. The voice
remains in its release lifecycle with an updated trajectory. Energy lost before the
catch remains lost. Finished and stolen voices remain finished.

## Same-offset order

Within stable input sequence at one sample offset:

1. Apply panic/reset and activation-boundary policy.
2. Store the exact controller value.
3. Update eligible dynamic release trajectories.
4. If the event targets the declared binary sustain controller, evaluate its edge.
5. On an up edge, release newly undeferred note voices and emit each effective
   `release` route exactly once.
6. Resolve pedal transition mechanics after deferred release routes.

An event that would exceed a fixed action budget is rejected atomically and counted;
partial controller, voice, or trigger state is forbidden.

## Activation-generation ownership

A releasing voice retains the immutable release-control declaration and compiled
curve belonging to its attack generation. Controller events continue to update that
voice through the shared bounded voice pool even after a new Performance generation
activates. New notes use the new generation. No update may dereference retired route
or curve storage.

## Compatibility policy

- Project 6 / authoring 5 and older native content migrates to controller 64,
  threshold 64, dynamic release disabled, and no release-control amount.
- Runtime instruments versions 1 through 4 retain current binary behavior.
- Existing pedal transition routes and Round Robin reset timing remain unchanged.
- Existing release routes still emit at most once.
- The `.drpkg` container does not change; HP-02 carries runtime instrument version 5
  through the existing authenticated package path.
- No half-pedal behavior is inferred from an instrument name, sample filename,
  articulation name, or use of CC64 controller conditions.

## Salamander SFZ conversion boundary

HP-02 converts the note-damping vocabulary used by the Accurate Salamander live
recommended preset:

- `sustain_cc`
- `sustain_lo`, including the ARIA import default of 0.5 when absent
- `ampeg_dynamic=1`
- `ampeg_releasecc64`
- `ampeg_release_curvecc64`
- `<curve> curve_index=N` and `v000` through `v127`

The known preset uses `sustain_cc=90`, `ampeg_releasecc64=100`, and curve 11. CC90
therefore owns binary sustain while CC64 controls the note release duration.

Pseudo pedal-resonance regions, volume modulation, CC72 release control, arbitrary
controller targets, high-resolution CC, smoothing opcodes, MPE, scripting, and
sympathetic resonance remain outside HP-01 through HP-05. Their importer findings
must remain explicit.

## Deterministic fixtures

`content/runtime/performance-engine/half-pedal/hp01/continuous-damper-timelines.json`
freezes six future behavioral timelines: continuous tail ordering, repedal catch,
release-trigger uniqueness, sustain-controller reassignment, activation cutover, and
legacy binary compatibility.

`synthetic-looped-piano.fixture.json` freezes the minimal route/sample topology for
future offline renders. Tests generate its constant sample data; no binary fixture is
required in HP-01.

`accurate-salamander-tests-cc64-trace.json` records the checked-in
`DemoMidi/AccurateSalamanderTests.mid` source by SHA-256. It contains 28 CC64 events
and 22 distinct values. The first expressive sweep rises only to 61 and returns to
zero, so Boolean DRS currently treats the complete sweep as pedal-up. The second
sweep crosses 64 only near its end.

## Direct-only expected-red seams

`drs_continuous_damper_hp01_red_tests` is intentionally excluded from CTest and
`drs_all_tests`. Invoke exactly one named seam; exit 1 is expected until its owning
later slice promotes it to registered green behavior:

- `preserve-continuous-cc64` — HP-03
- `dynamic-release-envelope` — HP-03
- `repedal-still-audible` — HP-04
- `release-trigger-uniqueness` — HP-04
- `sustain-controller-reassignment` — HP-02/HP-03
- `generation-owned-damper-update` — HP-03/HP-04
- `salamander-half-pedal-projection` — HP-02

Exit 2 means invalid invocation or fixture setup and is never an expected result.
Later slices must replace each red seam with behavioral coverage; deleting the seam
or treating expected exit 1 as a release pass is prohibited.

## HP-01 acceptance

1. The canonical contract constants compile and the registered fixture contract test
   passes.
2. All six synthetic timelines retain their required state/action markers.
3. The original MIDI parses as format 1, two tracks, 960 PPQ, and the exact frozen
   CC64 tick/value sequence matches the checked-in trace.
4. All seven named production seams return expected exit 1; unknown or misspelled
   seams return exit 2.
5. Existing production playback remains unchanged in HP-01.

## Implementation evidence — August 15, 2026

- Debug configuration and both HP-01 targets build successfully under the Visual
  Studio 2022 developer environment.
- `drs.continuous_damper.hp01.contract`,
  `drs.performance_engine.s0.fixtures`, and
  `drs.performance_engine.s7.pedal` pass together.
- Every named expected-red seam exits 1 with its seam identity; an unknown seam
  exits 2.
- The contract test verifies the source file SHA-256, raw format-1 header, two-track
  parse, 960 PPQ division, and all 28 trace events against both the frozen JSON and
  the original MIDI.

The checkout contains `AccurateSalamanderTests.mid` but not the Accurate Salamander
preset/sample corpus needed for a meaningful real-passage audio baseline. Capturing
that short current-engine reference render remains the only open HP-01 evidence item;
it does not block the frozen controller trace or later production seams.
