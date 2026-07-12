Message::setTransposeAmount(Number transposeValue) -> undefined

Thread safety: SAFE
Sets the transpose amount in semitones. Stored as int8 (-128 to 127). Does not change
getNoteNumber() -- transposed pitch is getNoteNumber() + getTransposeAmount(). When
EventIdHandler matches a note-off to its note-on, the transpose is automatically copied,
so setting it in onNoteOn is sufficient (no need to repeat in onNoteOff).

Anti-patterns:
  - In exported plugins (frontend builds), silently returns instead of throwing an error
    when called outside a mutable callback. Backend builds report an error. Code that
    accidentally calls this outside a callback may appear to work in exported plugins.

Pair with:
  getTransposeAmount -- read the current transpose
  setCoarseDetune -- typically paired with opposite sign for timbre shifting
  setNoteNumber -- alternative: directly change the raw note number

Source:
  ScriptingApi.cpp  Message::setTransposeAmount()
    -> messageHolder->setTransposeAmount((int8)transposeValue)
    -> FRONTEND_ONLY(return) -- silent no-op in exported plugins outside callback
