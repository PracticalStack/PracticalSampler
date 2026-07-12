Synth::addNoteOff(Integer channel, Integer noteNumber, Integer timeStampSamples) -> undefined

Thread safety: SAFE -- creates a HiseEvent on the stack, queries EventHandler for matching event ID, inserts into buffer, all lock-free.
Adds an artificial note-off to the MIDI buffer with explicit channel and sample-accurate timestamp.
Note-off velocity is hardcoded to 127. Matches note-on by channel and note number via getEventIdForNoteOff.

Anti-patterns:
  - Do NOT rely on this for overlapping notes on the same pitch -- matches by note number,
    not event ID. Use noteOffByEventId for unambiguous voice control.
  - Do NOT pass timestamp 0 expecting same-sample note-off -- clamped to minimum 1 sample
    via jmax(1, timeStampSamples). Use noteOffByEventId for immediate note-offs.

Pair with:
  addNoteOn -- the corresponding note-on method with explicit channel/timestamp
  noteOffByEventId -- preferred: unambiguous event-ID-based note-off

Source:
  ScriptingApi.cpp  Synth::addNoteOff()
    -> HiseEvent(NoteOff, noteNumber, 127, channel)
    -> setArtificial()
    -> getEventIdForNoteOff() for matching
    -> timestamp = jmax(1, timeStampSamples) + currentEvent.getTimeStamp()
    -> addHiseEventToBuffer()
