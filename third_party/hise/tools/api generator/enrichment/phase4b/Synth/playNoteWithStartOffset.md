Synth::playNoteWithStartOffset(Integer channel, Integer number, Integer velocity, Integer offset) -> Integer

Thread safety: SAFE -- delegates to internalAddNoteOn, all lock-free operations.
Plays an artificial note-on with an explicit MIDI channel and sample start offset, returning
the event ID. The start offset tells the sampler to begin playback at the specified sample
position within the audio file. Timestamp is fixed at 0.

Dispatch/mechanics:
  internalAddNoteOn(channel, number, velocity, 0, offset)
  -> offset clamped to UINT16_MAX (65535)
  -> HiseEvent(NoteOn) -> setArtificial() -> setStartOffset(offset)
  -> returns event ID

Anti-patterns:
  - Do NOT pass velocity 0 -- produces script error.
  - Do NOT pass offset > 65535 -- produces script error "Max start offset is 65536"
    (error message is off by one; actual max accepted is 65535).
  - Timestamp is fixed at 0. If you need both start offset AND non-zero timestamp, use
    addNoteOn + Message.setStartOffset() or construct via MessageHolder.

Source:
  ScriptingApi.cpp  Synth::playNoteWithStartOffset()
    -> internalAddNoteOn(channel, number, velocity, 0, offset)
