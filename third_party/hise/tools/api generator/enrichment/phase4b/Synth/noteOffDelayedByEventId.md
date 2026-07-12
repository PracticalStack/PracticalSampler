Synth::noteOffDelayedByEventId(Integer eventId, Integer timestamp) -> undefined

Thread safety: SAFE -- creates a HiseEvent on the stack, pops from EventHandler (fixed-size array lookup), inserts into MIDI buffer, all lock-free.
Sends a note-off for the specified event ID with a sample-accurate delay. The canonical note-off
method -- noteOffByEventId delegates here with timestamp=0.

Dispatch/mechanics:
  popNoteOnFromEventId((uint16)eventId) from EventHandler ring buffer
  If found: create NoteOff, set eventId, compute final timestamp, insert into MIDI buffer
  If already popped: setArtificialTimestamp(eventId, timestamp) to reschedule existing note-off
  HISE_USE_BACKWARDS_COMPATIBLE_TIMESTAMPS subtracts one block on audio thread

Anti-patterns:
  - Do NOT attempt to kill non-artificial events -- produces script error.
  - Calling twice on the same eventId does NOT send two note-offs -- the second call
    reschedules the timestamp of the first (by design for rescheduling scenarios).

Source:
  ScriptingApi.cpp  Synth::noteOffDelayedByEventId()
    -> popNoteOnFromEventId((uint16)eventId)
    -> HiseEvent(NoteOff) with matching channel/note
    -> addHiseEventToBuffer()
