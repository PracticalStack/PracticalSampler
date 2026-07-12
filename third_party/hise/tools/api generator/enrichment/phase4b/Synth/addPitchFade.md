Synth::addPitchFade(Integer eventId, Integer fadeTimeMilliseconds, Integer targetCoarsePitch, Integer targetFinePitch) -> undefined

Thread safety: SAFE -- creates a HiseEvent on the stack and inserts into MIDI buffer, no allocations, no locks.
Applies a pitch fade to an active voice by event ID. Smoothly transitions to the target pitch
over the specified time. No auto-kill behavior (unlike addVolumeFade).

Dispatch/mechanics:
  HiseEvent::createPitchFade(eventId, fadeTimeMs, coarse, fine)
  -> inherits current event's timestamp
  -> coarse/fine cast to uint8

Pair with:
  addNoteOn / playNote -- the note must be playing to apply a pitch fade
  addVolumeFade -- companion for volume fading on the same event
  noteOffByEventId -- to stop the note after pitch manipulation

Anti-patterns:
  - Do NOT pass negative semitone values -- targetCoarsePitch is cast to uint8, so negative
    values wrap silently. Use the two-phase glide pattern instead:
    addPitchFade(id, 0, -delta, 0) then addPitchFade(id, glideTime, 0, 0).

Source:
  ScriptingApi.cpp  Synth::addPitchFade()
    -> HiseEvent::createPitchFade(eventId, fadeTimeMs, (uint8)coarse, (uint8)fine)
    -> addHiseEventToBuffer()
