Message::getNoteNumber() -> Integer

Thread safety: SAFE
Returns the MIDI note number (0-127) of the current note-on or note-off event. Only valid
inside onNoteOn or onNoteOff callbacks. Returns the raw note number without transpose --
use getNoteNumber() + getTransposeAmount() for the transposed pitch.

Pair with:
  setNoteNumber -- modify the note number in-place
  getTransposeAmount -- get transpose offset (not included in raw note number)
  getVelocity -- typically read together for note processing

Source:
  ScriptingApi.cpp  Message::getNoteNumber()
    -> checks constMessageHolder->isNoteOnOrOff()
    -> constMessageHolder->getNoteNumber()
    -> returns uint8 number field from HiseEvent DWord 1
