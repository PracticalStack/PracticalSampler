MessageHolder::getControllerNumber() -> Integer

Thread safety: SAFE
Returns the controller number. Uses virtual CC numbers: 128 for PitchBend, 129 for
Aftertouch. For standard CC events returns 0-127. No event-type guard -- calling on
a non-controller event returns whatever is in the number byte.

Pair with:
  setControllerNumber -- set controller number (with type coercion for 128/129)
  getControllerValue -- read the controller value

Source:
  ScriptingApiObjects.cpp  ScriptingMessageHolder::getControllerNumber()
    -> (int)e.getControllerNumber()
    -> HiseEvent returns 128 for PitchBend, 129 for Aftertouch
