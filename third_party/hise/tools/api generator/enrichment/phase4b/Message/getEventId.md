Message::getEventId() -> Integer

Thread safety: SAFE
Returns the unique event ID (uint16, 0-65535) assigned by HISE's EventIdHandler.
Note-on and matching note-off share the same ID, enabling per-voice operations like
Synth.addVolumeFade() and Synth.addPitchFade(). Works on any event type.

Anti-patterns:
  - Do NOT assume older notes have lower IDs -- event IDs wrap around at 65536.

Source:
  ScriptingApi.cpp  Message::getEventId()
    -> constMessageHolder->getEventId()
    -> returns uint16 eventId from HiseEvent DWord 3
    -> FRONTEND_ONLY(return 0) -- silently returns 0 in exported plugins outside callback
