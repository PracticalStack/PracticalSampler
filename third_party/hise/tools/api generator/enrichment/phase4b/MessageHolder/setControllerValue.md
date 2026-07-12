MessageHolder::setControllerValue(Number newControllerValue) -> undefined

Thread safety: SAFE
Sets the controller value. If the event is currently PitchBend, stores as 14-bit
pitch wheel value (0-16383) split across number and value bytes. For all other
types, writes to the value byte as uint8. No event-type guard beyond the pitch
wheel check.

Dispatch/mechanics:
  e.isPitchWheel() -> e.setPitchWheelValue(val) -- 14-bit split: number = val & 127, value = (val >> 7) & 127
  otherwise -> e.setControllerValue((uint8)val)

Pair with:
  getControllerValue -- read the value (14-bit for PitchBend, uint8 otherwise)
  setControllerNumber -- set the controller number first

Source:
  ScriptingApiObjects.cpp  ScriptingMessageHolder::setControllerValue()
    -> PitchWheel: e.setPitchWheelValue()
    -> Other: e.setControllerValue()
