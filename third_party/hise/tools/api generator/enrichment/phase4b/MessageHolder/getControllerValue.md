MessageHolder::getControllerValue() -> Integer

Thread safety: SAFE
Returns the controller value. For PitchBend events, returns the full 14-bit pitch
wheel value (0-16383). For all other types, returns the value byte as int. No
event-type guard beyond the pitch wheel check.

Dispatch/mechanics:
  e.isPitchWheel() ? e.getPitchWheelValue() : (int)e.getControllerValue()
  Pitch wheel value reconstructed from: number | (value << 7)

Pair with:
  setControllerValue -- set the controller value
  getControllerNumber -- read the controller number

Source:
  ScriptingApiObjects.cpp  ScriptingMessageHolder::getControllerValue()
    -> PitchWheel: e.getPitchWheelValue() (14-bit)
    -> Other: (int)e.getControllerValue() (uint8)
