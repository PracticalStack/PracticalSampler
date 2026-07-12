Synth::addNoteOn(Integer channel, Integer noteNumber, Integer velocity, Integer timeStampSamples) -> Integer

Thread safety: SAFE -- creates a HiseEvent on the stack, registers with EventHandler (fixed-size array write), inserts into MIDI buffer, all lock-free.
Adds an artificial note-on with explicit channel, velocity, and sample-accurate timestamp. Returns event ID.
Store the returned ID for later use with noteOffByEventId or addVolumeFade.

Dispatch/mechanics:
  Delegates to internalAddNoteOn(channel, noteNumber, velocity, timeStampSamples, 0)
  -> HiseEvent(NoteOn, noteNumber, velocity, channel) -> setArtificial()
  -> EventHandler.pushArtificialNoteOn() + Message.pushArtificialNoteOn()
  -> addHiseEventToBuffer() -> returns event ID
  HISE_USE_BACKWARDS_COMPATIBLE_TIMESTAMPS subtracts one block on audio thread

Pair with:
  noteOffByEventId / noteOffDelayedByEventId -- stop the note using the returned event ID
  addVolumeFade -- fade out and optionally auto-kill with targetVolume -100

Anti-patterns:
  - Do NOT discard the returned event ID -- without it, you cannot reliably stop the note.
    addNoteOff matches by note number which is ambiguous with overlapping notes.
  - Unlike playNote, addNoteOn accepts velocity 0. A zero-velocity note-on may produce
    silent voices depending on the synth configuration.

Source:
  ScriptingApi.cpp  Synth::addNoteOn()
    -> internalAddNoteOn(channel, noteNumber, velocity, timeStampSamples, 0)
