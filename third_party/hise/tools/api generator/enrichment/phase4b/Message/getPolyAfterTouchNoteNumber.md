Message::getPolyAfterTouchNoteNumber() -> Integer

Thread safety: SAFE
Returns the note number associated with a polyphonic aftertouch event. Only valid on
an aftertouch event inside onController. Reports an error if the event is not aftertouch.

Anti-patterns:
  - [BUG] Accesses mutable messageHolder internally despite being a const getter. In
    read-only contexts (e.g., voice start modulators), the null check on constMessageHolder
    passes but the method dereferences null messageHolder -- undefined behavior. Use
    getControllerNumber() and getControllerValue() as safer alternatives in read-only contexts.

Source:
  ScriptingApi.cpp  Message::getPolyAfterTouchNoteNumber()
    -> checks constMessageHolder->isAftertouch()
    -> messageHolder->getNoteNumber() [BUG: uses mutable pointer]
