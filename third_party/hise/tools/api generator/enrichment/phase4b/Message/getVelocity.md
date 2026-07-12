Message::getVelocity() -> Integer

Thread safety: SAFE
Returns the velocity (0-127) of the current note-on or note-off event. Only valid inside
onNoteOn or onNoteOff callbacks. Works on both note-on and note-off (unlike setVelocity
which is note-on only).

Pair with:
  setVelocity -- set velocity (note-on only)
  getNoteNumber -- typically read together
  getGain -- alternative gain control that works on any event type

Source:
  ScriptingApi.cpp  Message::getVelocity()
    -> checks constMessageHolder->isNoteOnOrOff()
    -> constMessageHolder->getVelocity()
    -> returns uint8 value field from HiseEvent DWord 1
