MessageHolder::getVelocity() -> Integer

Thread safety: SAFE
Returns the velocity (value byte as uint8, 0-255; standard MIDI is 0-127). No
event-type guard -- on non-note events returns whatever is in the value byte
(e.g., controller value for CC, pressure for aftertouch).

Pair with:
  setVelocity -- set the velocity
  getNoteNumber -- read the note number

Source:
  ScriptingApiObjects.cpp  ScriptingMessageHolder::getVelocity()
    -> (int)e.getVelocity()
