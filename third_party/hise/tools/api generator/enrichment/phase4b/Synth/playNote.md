Synth::playNote(Integer noteNumber, Integer velocity) -> Integer

Thread safety: SAFE -- delegates to internalAddNoteOn which creates a HiseEvent on the stack, registers with EventHandler, inserts into MIDI buffer, all lock-free.
Plays an artificial note-on and returns its event ID. Simplest note generation method -- uses
fixed defaults: channel 1, timestamp 0, start offset 0. Velocity 0 is rejected with script error.

Dispatch/mechanics:
  internalAddNoteOn(1, noteNumber, velocity, 0, 0)
  -> HiseEvent(NoteOn) -> setArtificial()
  -> EventHandler.pushArtificialNoteOn() + Message.pushArtificialNoteOn()
  -> returns event ID

Pair with:
  noteOffByEventId / noteOffDelayedByEventId -- stop the note using the returned event ID
  addVolumeFade -- fade out and optionally auto-kill with targetVolume -100
  addNoteOn -- variant with explicit channel and timestamp

Anti-patterns:
  - Do NOT discard the returned event ID -- without it, there is no reliable way to stop the note.
  - Do NOT pass velocity 0 -- produces script error "A velocity of 0 is not valid!".
    Use noteOffByEventId to stop notes.
  - Channel is hardcoded to 1. Use addNoteOn if you need a specific MIDI channel.

Source:
  ScriptingApi.cpp  Synth::playNote()
    -> internalAddNoteOn(1, noteNumber, velocity, 0, 0)
