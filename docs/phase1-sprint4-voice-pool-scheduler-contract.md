# Sprint 4.3 Fixed Voice Pool And Event Scheduler Contract

Status: authoritative Mini Sprint 4.3 scheduling contract, completed July 19, 2026.

## Fixed capacities

Each `SamplerVoicePool` owns exactly 24 inline voice slots. No vector, heap-backed free list, or
capacity growth exists in the scheduling/render path. Each `SamplerEventBlock` owns exactly 128
inline events. These values match the Sprint 4 entry-gate callback profile per playback context.

Slots have explicit `free`, `active`, `releasing`, and `finished` states. A finished slot remains
observable until reused or reset; reuse of free/finished state is not reported as a steal.

## Event scratch and ordering

`SamplerEventBlock::push()` inserts events by ascending sample offset using bounded in-place moves.
Equal-offset events retain insertion order. The 129th event is rejected and increments the block's
drop counter without allocation or growth. `clear()` resets count/drop state without releasing
storage.

`renderBlock()` accepts only a valid output view, at most 128 events, nondecreasing event offsets,
and offsets inside `[0, frameCount)`. Invalid raw event views are rejected before any voice renders
or event mutates state.

For each event, the pool:

1. renders the half-open output range from the previous offset to the event offset;
2. applies the event at that exact offset;
3. preserves caller order for another event at the same offset;
4. renders the final half-open range through the block end.

Output remains additive.

## Note and command semantics

- Note-on validates normalized velocity, converts it deterministically to MIDI 1–127, selects a
  matching normalized route, and starts one new voice.
- Repeated note-ons own separate voice slots.
- Note-off begins the compatibility release on every active voice with the matching source MIDI note.
- Ordinary all-notes-off begins the compatibility release on every active voice.
- Emergency reset immediately resets active/releasing voices to `free`, producing silence from that
  event offset onward.

Mini Sprint 4.4 now supplies the release envelope and completion law under
[the lifecycle contract](phase1-sprint4-voice-lifecycle-contract.md), preserving these 4.3
sample-accurate state transitions.

## Route selection

The current normalized-route selector filters by key and velocity range, then chooses deterministically
by closest root key, narrowest key range, narrowest velocity range, and lexical zone identity. Mini
Sprint 4.6 will supply articulation/macro-normalized activation routes without moving selection back
into the shell.

## Allocation and stealing

Allocation order is fixed:

1. lowest-index free or finished slot;
2. oldest releasing voice (lowest stable voice ID);
3. oldest active voice (lowest stable voice ID).

Voice IDs increase monotonically and zero is reserved as invalid. The 25th simultaneous note steals
exactly one voice; the pool never grows. Failed/no-route/invalid note admission increments the primitive
drop counter.

## Real-time behavior

`prepare()` runs before callback use and retains only a non-owning model pointer plus output sample
rate. `renderBlock()`, event admission, range rendering, state counting, allocation, stealing,
note-off, all-notes-off, and reset are `noexcept`/bounded operations with no locks or ownership
exchange. The focused test installs global allocation/deallocation probes and requires zero events at
128-event/64-voice pressure.

The Sprint 4.5 [playback context](phase1-sprint4-playback-context-contract.md) retains every immutable
model activation until no active or releasing pool voice references it, then returns a primitive
retirement token for message-owned reclamation.
