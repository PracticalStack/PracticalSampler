Message::getPolyAfterTouchPressureValue() -> Integer

Thread safety: SAFE
Returns the pressure value (0-127) of a polyphonic aftertouch event. Only valid on an
aftertouch event inside onController.

Anti-patterns:
  - [BUG] Same null-pointer issue as getPolyAfterTouchNoteNumber -- accesses mutable
    messageHolder in a const getter. Undefined behavior in read-only contexts. Use
    getControllerValue() as a safer alternative for aftertouch pressure.

Source:
  ScriptingApi.cpp  Message::getPolyAfterTouchPressureValue()
    -> checks constMessageHolder->isAftertouch()
    -> messageHolder->getAfterTouchValue() [BUG: uses mutable pointer]
