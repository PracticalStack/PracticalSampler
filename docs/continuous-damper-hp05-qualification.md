# HP-05 — Real-passage qualification and compatibility

Status: DRS implementation and internal Release qualification complete on
August 15, 2026. The external Plogue perceptual A/B remains pending because no
Plogue render or reproducible reference session is present in the workspace.

## Live-preset import and package

`drs.continuous_damper.hp05.qualification` analyzes and projects the real Accurate
Salamander `sfz_live` recommended preset. It requires:

- all focused half-pedal opcodes to classify as converted;
- `ampeg_release_oncc72` and `volume_oncc23` to remain report-only at a visible
  confirmation gate;
- 1,704 analyzed regions, 1,700 projected routes, 637 sources, and the same four
  omitted unsafe random regions as the approved large-corpus boundary;
- exactly 1,408 note-on routes with curve 11 and the authored CC90/CC64/100-second
  control law; and
- the 135 curve-12 pseudo-resonance routes to remain distinguishable from the
  curve-11 note regions.

The test applies the reviewed projection, writes an instrument-schema-5 metadata
package, authenticates it on reopen, and proves that all 1,408 curve-11 note routes
survive. The Release large-instrument gate separately exported and reopened the
complete 2.63 GB package with the real WAV corpus. See
`artifacts/continuous-damper/hp05/accurate-salamander-live-package-qualification.md`.

## Reported MIDI passage

The isolated reported passage retains 125 note-ons, 125 explicit note-offs, and 47
exact CC64 events. The qualification preserves event sample position, MIDI channel,
controller number/value, note-off velocity, and stable input order in the common
offline render harness. It appends one all-notes-off event 10 ms after the source
endpoint to produce a bounded, comparable release window.

A direct Performance-pool regression also renders the passage through the reported
2.46–2.48 second pedal transition with Salamander's authored one-second base
release. The former 24-voice profile had already stolen 12 voices and lost at least
one of F3, A#3, and D4. The qualified 64-voice per-context profile reaches that
transition with zero steals, all three chord notes still live, and a repedal catch.

The real passage is rendered at 44.1 and 48 kHz with 128-, 256-, and 512-frame
blocks. At each sample rate, all three partitionings are sample-equivalent. Both
rates produce the same lifecycle evidence:

| Metric | 44.1 kHz | 48 kHz |
|---|---:|---:|
| Started/released voices | 125/125 | 125/125 |
| Dynamic release updates | 125 | 125 |
| Repedal catches | 38 | 38 |
| Continuous tail RMS | 0.353919482 | 0.353918568 |
| Legacy tail RMS | 0.249218649 | 0.249217461 |
| 128-frame checksum | `bc5de62fe5671984` | `c122e80be954a634` |

The continuous result retains roughly 1.42 times the tail RMS of the frozen legacy
binary path over the CC-aligned comparison window. No MIDI event is dropped, and
the continuous tail remains audible through the bounded render. This is the
automated evidence that the intermediate-value cutoff is removed; it is not a
claim of acoustic parity with Plogue.

## Compatibility and scope

The existing binary-sustain, performance-event, release-trigger, generation
ownership, realtime-safety, package/session, SFZ, streaming, UI responsiveness,
standalone, VST3, and editor open/closed gates remain the compatibility authority.
The large-instrument responsiveness fixture now uses `sfz_live`, so future UI
baseline runs exercise the enabled half-pedal document rather than `sfz_daw`.

That live-preset run exposed and closed one host-state boundary: a 1,700-route
schema-7 project is larger than the original 1.5 MiB embedded-snapshot ceiling
because every route persists its immutable 128-point curve. The bounded host-state
limits are now 7.5 MiB for the project snapshot and 8 MiB for the complete chunk.
Sample/stream bytes are still excluded, and the parser still rejects a payload one
byte over the limit before JSON expansion. Projects containing only the legacy
binary-default damper shape retain their schema-6 canonical digest identity.

The Debug `sfz_live` responsiveness authority passed with 46 us zone interaction
during background state publication, 6.12 s worker serialization, 418/141 us
Preview/Publish dispatch, both editor keyboard and host MIDI audible, 180 seconds
of accelerated concurrent playback, a 6.852 ms maximum message dispatch, and a
699 us maximum package callback.

The Release authority also passed: 15 us interaction during publication, 473 ms
host-state serialization, 49/15 us Preview/Publish dispatch, 1.56 s full-draft
Preview readiness, 28.091 ms maximum concurrent dispatch, and a 417 us maximum
package callback. Release standalone and VST3 targets linked, and the compiled
VST3 host scan/state qualification passed in 2.26 s.

Final automated matrix:

- HP-01 through HP-05: 5/5 passed in Debug; HP-05 also passed in Release.
- Offline renderer, SFZ compatibility/projection/runtime/determinism,
  voice-generation ownership, shell publish parity, realtime safety, and package
  session: 10/10 passed after the host-state boundary repair.
- The 64-voice per-context / 128-voice combined callback profile passed the complete
  44.1/48 kHz and 32–1024-frame realtime guard without allocation, lock, I/O, or
  deadline failures.
- Performance engine S0 through S10: 11/11 passed.
- Host-state contract, restore coordinator, recovery UI, restore stress, project
  recall, and phase-1 state recall: 6/6 passed; background publication also passed.
- `sfz_live` UI responsiveness: Debug and Release passed.
- Full live-corpus package and compiled Release VST3 qualification: passed.

HP-05 adds no pedal-down resonance, pedal-up resonance, sympathetic string
coupling, soundboard model, convolution response, arbitrary controller modulation,
or blanket Plogue opcode parity. Curve-12 pseudo-resonance content is inventoried;
its unsupported modulation remains explicit and deferred to the separate resonance
iteration.

## Remaining external sign-off

The generated qualification JSON records
`PASS_WITH_REFERENCE_PENDING`, `plogueReferenceAvailable=false`, and
`PENDING_EXTERNAL_REFERENCE`. A final perceptual sign-off requires an aligned
Plogue render of the frozen MIDI passage (or a reproducible Plogue session and
export procedure). Until that input exists, DRS behavior is internally qualified
but Plogue parity is deliberately not asserted.
