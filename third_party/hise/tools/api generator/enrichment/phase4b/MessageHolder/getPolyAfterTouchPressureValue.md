MessageHolder::getPolyAfterTouchPressureValue() -> Integer

Thread safety: SAFE
Returns the pressure value from a polyphonic aftertouch event. Reads the value byte
as uint8 (0-255). No event-type guard -- on non-aftertouch events, returns whatever
is in the value byte.

Pair with:
  getPolyAfterTouchNoteNumber -- read the associated note number
  setPolyAfterTouchNoteNumberAndPressureValue -- set both note and pressure

Source:
  ScriptingApiObjects.cpp  ScriptingMessageHolder::getPolyAfterTouchPressureValue()
    -> (int)e.getAfterTouchValue()
