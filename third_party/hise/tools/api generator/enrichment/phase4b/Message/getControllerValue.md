Message::getControllerValue() -> Integer

Thread safety: SAFE
Returns the value of the current controller-type event. Return range depends on type:
CC returns 0-127, Aftertouch returns 0-127, PitchBend returns 0-16383 (14-bit).

Dispatch/mechanics:
  Internally dispatches to different HiseEvent accessors based on event type:
    Controller -> getControllerValue() (0-127)
    Aftertouch -> getAfterTouchValue() (0-127)
    PitchBend  -> getPitchWheelValue() (0-16383, encoded as number | (value << 7))

Anti-patterns:
  - Do NOT normalize by dividing by 127.0 for all events -- pitch bend returns 0-16383,
    producing values up to ~129. Check getControllerNumber() first to determine the range.

Source:
  ScriptingApi.cpp  Message::getControllerValue()
    -> branches on isController/isAftertouch/isPitchWheel
    -> each calls the corresponding HiseEvent accessor
