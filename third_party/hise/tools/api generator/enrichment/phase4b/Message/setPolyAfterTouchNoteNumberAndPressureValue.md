Message::setPolyAfterTouchNoteNumberAndPressureValue(Number noteNumber, Number aftertouchAmount) -> undefined

Thread safety: SAFE
Sets both the note number and pressure value of a polyphonic aftertouch event in a single
call. Only valid on an aftertouch event inside a mutable callback. Both values cast to
uint8 (0-127). The event type check runs unconditionally.

Anti-patterns:
  - Both mono and poly aftertouch use the same Type::Aftertouch internally. The check
    uses isAftertouch() which returns true for both subtypes. Calling this on a channel
    pressure event silently converts it to polyphonic format by writing the note number.

Pair with:
  getPolyAfterTouchNoteNumber -- read the note number
  getPolyAfterTouchPressureValue -- read the pressure value

Source:
  ScriptingApi.cpp  Message::setPolyAfterTouchNoteNumberAndPressureValue()
    -> checks messageHolder->isAftertouch() (unconditional)
    -> messageHolder->setNoteNumber((uint8)noteNumber)
    -> messageHolder->setAfterTouchValue((uint8)aftertouchAmount)
