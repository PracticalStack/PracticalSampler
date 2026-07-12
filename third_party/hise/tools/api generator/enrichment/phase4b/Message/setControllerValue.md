Message::setControllerValue(Number newControllerValue) -> undefined

Thread safety: SAFE
Sets the CC value on the current event. Only works on Controller events -- reports an error
for PitchBend and Aftertouch. Value cast to uint8 (0-127, higher values wrap).

Anti-patterns:
  - Unlike getControllerValue() which works on Controller/PitchBend/Aftertouch,
    setControllerValue() only works on Controller. To modify PitchBend value, there is
    no direct setter. For Aftertouch, use setMonophonicAfterTouchPressure() or
    setPolyAfterTouchNoteNumberAndPressureValue().

Pair with:
  getControllerValue -- read the current value
  setControllerNumber -- typically set together

Source:
  ScriptingApi.cpp  Message::setControllerValue()
    -> checks messageHolder->isController() (rejects PitchBend/Aftertouch)
    -> messageHolder->setControllerValue((uint8)newControllerValue)
