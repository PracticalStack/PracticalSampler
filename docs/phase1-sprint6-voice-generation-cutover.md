# Mini Sprint 6.6 Voice Generation Cutover Contract

Date: July 20, 2026  
Status: Implemented

## Default cutover policy

Every accepted Performance activation receives a non-zero, monotonically advancing context
generation. A voice copies that generation when its note-on starts and retains the immutable render
model, route, decoded sample handle, pitch, loop, gain, and pan associated with that generation for
its complete active and release lifetime.

The audio callback applies a pending activation before consuming the first event in the block.
Consequently, a note-on at sample zero of the activation block belongs to the new generation;
voices started before the boundary remain on their original generation. Incompatible or removed
routes complete through the old immutable model. Mini Sprint 6.6 does not add crossfade DSP.

## Event ownership

- Note-off examines only Performance voices whose source note matches. Each matching voice begins
  release through its own retained generation; the event never substitutes the active model.
- Sustain-down defers matching note-offs without changing generation. Sustain-up releases every
  deferred Performance voice through its original generation.
- All-notes-off starts release for active voices across all Performance generations. All-sound-off
  and reset clear every Performance voice immediately.
- Performance and Preview retain separate contexts, event queues, generations, pools, and reset
  paths. No Performance event can release or steal a Preview voice.

## Fixed capacity and stealing

The Performance pool remains fixed at 64 voices and activation ownership remains fixed at four
slots. A note-on never grows either structure. Under pressure, Performance selects deterministically:

1. oldest active voice from a retired generation;
2. oldest active voice from the current generation;
3. oldest releasing voice from a retired generation;
4. oldest releasing voice from the current generation.

Generation then voice identity breaks ties. This makes old active work yield before release tails;
only an all-releasing pool can require a release-tail steal. Preview retains its established
release-first compatibility order.

Rapid activations can retain at most four simultaneously leased render models. A fifth generation
is rejected while all slots remain voice-owned, preserving the active generation. After voices
finish and message-owned retirement drains, a newer generation may stage.

## Lifetime and diagnostics

Old render models and decoded samples remain reachable through voice generation leases. The audio
thread returns only bounded retirement tokens. The message thread performs final model and payload
release after the last voice for that generation ends.

Context and processor diagnostics expose the active generation, active-generation and retired-
generation voice counts, sustain-deferred count, total steals, cross-generation steals, and
releasing-voice steals. Per-slot test snapshots expose voice ID, generation, model revision, route
pitch increment, gain, loop state, and sustain deferral without transferring ownership.

## Owned limits

- Performance voices: 64
- activation slots / simultaneous leased generations: 4
- retirement token queue: 8 plus the ring sentinel
- event block: 128

All generation selection, sustain bookkeeping, event routing, and stealing remain fixed-capacity
and allocation-free on the audio callback.
