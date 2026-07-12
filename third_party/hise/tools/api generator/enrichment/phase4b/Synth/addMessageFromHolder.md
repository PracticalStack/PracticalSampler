Synth::addMessageFromHolder(ScriptObject messageHolder) -> Integer

Thread safety: SAFE -- creates a HiseEvent copy on the stack, registers with EventHandler, inserts into MIDI buffer, all lock-free.
Inserts a MIDI event from a MessageHolder into the processing buffer. Event is marked artificial.
Return value depends on event type: event ID for note-on, timestamp for note-off, 0 for other events.

Dispatch/mechanics:
  Copies event from ScriptingMessageHolder -> setArtificial()
  Note-on: pushArtificialNoteOn() to EventHandler + Message, returns event ID
  Note-off: getEventIdForNoteOff() for matching, returns timestamp
  Other: adds to buffer, returns 0

Anti-patterns:
  - Do NOT assume the return value is always an event ID -- it changes meaning based on
    event type. Check the event type before interpreting the return value.
  - For note-off events, if no matching note-on was registered, the note-off gets event
    ID 0 and may fail to stop the intended voice.

Source:
  ScriptingApi.cpp  Synth::addMessageFromHolder()
    -> ScriptingMessageHolder copy -> setArtificial()
    -> branch on isNoteOn/isNoteOff/other
