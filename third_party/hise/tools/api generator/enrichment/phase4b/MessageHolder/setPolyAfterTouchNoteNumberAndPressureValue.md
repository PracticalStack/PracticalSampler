MessageHolder::setPolyAfterTouchNoteNumberAndPressureValue(Number noteNumber, Number aftertouchAmount) -> undefined

Thread safety: SAFE
Sets both note number and pressure for polyphonic aftertouch in one call. Both
values cast to uint8 (0-255). This is the only way to set poly aftertouch data --
setNoteNumber() clamps to 0-127 and has different semantics. Set the type to
Aftertouch first for a proper poly aftertouch event.

Required setup:
  const var mh = Engine.createMessageHolder();
  mh.setType(mh.Aftertouch);

Pair with:
  getPolyAfterTouchNoteNumber -- read the note number
  getPolyAfterTouchPressureValue -- read the pressure value
  setMonophonicAfterTouchPressure -- alternative for channel pressure

Source:
  ScriptingApiObjects.cpp  ScriptingMessageHolder::setPolyAfterTouchNoteNumberAndPressureValue()
    -> e.setAfterTouchValue((uint8)noteNumber, (uint8)aftertouchAmount)
