Synth::noteOffByEventId(Integer eventId) -> undefined

Thread safety: SAFE -- delegates to noteOffDelayedByEventId(eventId, 0), all lock-free operations.
Sends a note-off for the specified event ID with zero delay. The primary method for stopping
artificial notes from playNote, addNoteOn, or playNoteWithStartOffset.

Dispatch/mechanics:
  Delegates to noteOffDelayedByEventId(eventId, 0)
  -> pops note-on from EventHandler ring buffer
  -> creates NoteOff with matching channel/note/eventId
  -> if already popped, calls setArtificialTimestamp to update existing note-off

Pair with:
  playNote / addNoteOn / playNoteWithStartOffset -- create the note to stop
  noteOffDelayedByEventId -- variant with sample-accurate delay
  addVolumeFade -- alternative: fade out then auto-kill with targetVolume -100

Anti-patterns:
  - Do NOT use to stop real (non-artificial) events -- produces script error "Hell breaks
    loose if you kill real events artificially!".

Source:
  ScriptingApi.cpp  Synth::noteOffByEventId()
    -> noteOffDelayedByEventId(eventId, 0)
