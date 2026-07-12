MessageHolder::getPolyAfterTouchNoteNumber() -> Integer

Thread safety: SAFE
Returns the note number from a polyphonic aftertouch event. Reads the number byte as
uint8. No event-type guard -- on non-aftertouch events, returns whatever is in the
number byte.

Pair with:
  getPolyAfterTouchPressureValue -- read the pressure value
  setPolyAfterTouchNoteNumberAndPressureValue -- set both note and pressure

Source:
  ScriptingApiObjects.cpp  ScriptingMessageHolder::getPolyAfterTouchNoteNumber()
    -> (int)e.getAfterTouchNumber()
