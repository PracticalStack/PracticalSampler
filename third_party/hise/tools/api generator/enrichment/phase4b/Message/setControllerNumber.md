Message::setControllerNumber(Number newControllerNumber) -> undefined

Thread safety: SAFE
Sets the MIDI CC number on the current event. Only works on Controller events -- reports
an error for PitchBend, Aftertouch, and all other event types. Value cast to uint8 (0-127).

Anti-patterns:
  - Unlike getControllerNumber() which works on Controller, PitchBend, and Aftertouch,
    setControllerNumber() only works on Controller. You cannot remap a pitch wheel event
    to a CC number -- the setter rejects non-Controller types.

Pair with:
  getControllerNumber -- read the current CC number
  setControllerValue -- typically set together

Source:
  ScriptingApi.cpp  Message::setControllerNumber()
    -> checks messageHolder->isController() (rejects PitchBend/Aftertouch)
    -> messageHolder->setControllerNumber((uint8)newControllerNumber)
