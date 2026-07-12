Synth::getNumPressedKeys() -> Integer

Thread safety: SAFE -- reads an Atomic<int> via .get(), single atomic load.
Returns the number of currently pressed MIDI keys (real note-ons without matching note-offs).
Only counts real (non-artificial) key presses -- script-generated notes are excluded.

Pair with:
  isLegatoInterval -- check if current note is a legato transition
  isKeyDown -- check a specific note number

Source:
  ScriptingApi.cpp  Synth::getNumPressedKeys()
    -> numPressedKeys.get()
