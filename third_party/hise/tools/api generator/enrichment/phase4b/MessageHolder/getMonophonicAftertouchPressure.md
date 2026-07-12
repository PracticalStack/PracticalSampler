MessageHolder::getMonophonicAftertouchPressure() -> Integer

Thread safety: SAFE
Returns the channel pressure (monophonic aftertouch) value. Reads the value byte as
uint8 (0-255). No event-type guard -- if called on a non-aftertouch event, returns
whatever is in the value byte (e.g., velocity for NoteOn).

Pair with:
  setMonophonicAfterTouchPressure -- set channel pressure
  isMonophonicAfterTouch -- check if event is aftertouch type

Source:
  ScriptingApiObjects.cpp  ScriptingMessageHolder::getMonophonicAftertouchPressure()
    -> (int)e.getChannelPressureValue()
