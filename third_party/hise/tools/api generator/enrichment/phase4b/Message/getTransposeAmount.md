Message::getTransposeAmount() -> Integer

Thread safety: SAFE
Returns the transpose amount in semitones (int8). Does not alter getNoteNumber() --
the transposed pitch is getNoteNumber() + getTransposeAmount(). When EventIdHandler
matches a note-off to its note-on, the transpose is automatically copied to the note-off.

Anti-patterns:
  - In exported plugins (frontend builds), silently returns 0 instead of throwing an error
    when called outside a MIDI callback. Backend builds report an error. Code that
    accidentally calls this outside a callback may work in exported plugins but fail
    during development.

Pair with:
  setTransposeAmount -- set the transpose value
  getNoteNumber -- raw note number (transpose not included)

Source:
  ScriptingApi.cpp  Message::getTransposeAmount()
    -> constMessageHolder->getTransposeAmount()
    -> FRONTEND_ONLY(return 0) -- silent default in exported plugins
