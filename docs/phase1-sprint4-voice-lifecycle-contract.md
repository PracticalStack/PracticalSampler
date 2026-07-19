# Sprint 4.4 Loop, Release, And Voice Lifecycle Contract

Status: authoritative Mini Sprint 4.4 lifecycle contract, completed July 19, 2026.

## Forward-loop semantics

Enabled loops are prevalidated half-open source-frame ranges `[loopStartFrame, loopEndFrame)`.
`loopStartFrame` is included and `loopEndFrame` is the first excluded frame.

- A voice starting before the loop progresses normally into it.
- A voice starting inside the loop begins at its authored start frame and wraps normally.
- A voice starting at or after the loop end plays the natural post-loop tail and does not jump
  backward into the loop.
- At the final frame inside the loop, linear interpolation uses the loop-start frame as its next
  sample rather than reading the post-loop tail.
- After cursor advancement, fractional overshoot is preserved with modulo loop length. One source
  increment may cross the boundary once or multiple times without losing phase.
- A valid one-frame loop repeats that one frame deterministically.
- An active loop does not end naturally. During release, it continues wrapping until the release
  envelope completes.

Invalid/empty/out-of-range loops remain rejected by `buildSamplerRenderModel()` before audio.

## Compatibility release law

`SamplerVoice::beginRelease()` transitions an active voice to `releasing` exactly once. Repeated
note-off calls do not restart or lengthen the envelope.

The compatibility release is exactly 2,048 output samples:

```text
envelope = releaseSamplesRemaining / 2048
```

The first release sample uses `2048/2048` (unity), the second uses `2047/2048`, and the final sample
uses `1/2048`. Remaining count decrements after each mixed sample. The voice transitions to
`finished` immediately after the final release sample. Cursor/interpolation and envelope evolution
are independent of host block partitioning.

## Voice and slot lifecycle

- `start()` moves an idle voice to active.
- A non-looping active voice finishes after its final retained PCM frame.
- Note-off/all-notes-off move matching active voices and pool slots to releasing.
- Release completion moves the slot to explicit finished state.
- Free and finished slots are reused before any steal.
- A steal resets the selected releasing/active voice before starting the replacement.
- Emergency reset immediately resets active/releasing voices and slots to free.
- Repeated reset after the first clear is idempotent.

Started, released, naturally/release-completed, stolen, dropped, active, releasing, finished, and
reset counts remain primitive fields in `SamplerVoicePoolRenderResult`/`SamplerRenderResult`.

## Ownership and real-time behavior

Voice finish, steal, and reset clear only primitive state and non-owning raw pointers. They never
copy, decrement, or destroy the immutable render model, activation payload, prepared handles, or PCM.
The focused lifecycle matrix verifies the external model `shared_ptr` use count remains unchanged
through release stealing and repeated reset. The Sprint 4.5
[playback context](phase1-sprint4-playback-context-contract.md) is the explicit model/activation
lease owner and returns primitive retirement tokens to the message-owned reclamation path.

All lifecycle methods remain bounded and `noexcept`; no allocation, lock, file access, path lookup,
decode, string construction, or shared-resource destruction is introduced in the render path.
