Synth::isKeyDown(Integer noteNumber) -> Integer

Thread safety: SAFE -- reads a bit from a BigInteger bitfield, single array element access.
Returns true if the specified MIDI note is currently held down. Only tracks real (non-artificial)
MIDI events -- script-generated notes via playNote/addNoteOn are NOT reflected.

Source:
  ScriptingApi.cpp  Synth::isKeyDown()
    -> keyDown[noteNumber]
