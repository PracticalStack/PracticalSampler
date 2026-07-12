MessageHolder::setMonophonicAfterTouchPressure(Number pressure) -> undefined

Thread safety: SAFE
Sets the channel pressure (monophonic aftertouch) value. Writes to the value byte as
uint8 (0-255). No event-type guard -- on non-aftertouch events, overwrites whatever
the value byte represents. Set the type to Aftertouch first for a proper event.

Pair with:
  getMonophonicAftertouchPressure -- read channel pressure
  isMonophonicAfterTouch -- check event type
  setPolyAfterTouchNoteNumberAndPressureValue -- alternative for poly aftertouch

Source:
  ScriptingApiObjects.cpp  ScriptingMessageHolder::setMonophonicAfterTouchPressure()
    -> e.setChannelPressureValue((uint8)pressure)
