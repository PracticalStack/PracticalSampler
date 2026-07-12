Synth::isLegatoInterval() -> Integer

Thread safety: SAFE -- reads an Atomic<int> via .get() and compares to 1.
Returns true if the current note is a legato transition (another key already held down).
Implementation: numPressedKeys != 1. Returns true when 0 or 2+ keys are pressed.

Anti-patterns:
  - Do NOT use in onNoteOff to detect "last key released" -- returns true when 0 keys are
    pressed (numPressedKeys == 0). Check getNumPressedKeys() == 0 instead.
  - Only real MIDI events affect the count. Artificial notes do not change numPressedKeys.

Source:
  ScriptingApi.cpp  Synth::isLegatoInterval()
    -> numPressedKeys.get() != 1
