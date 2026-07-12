AudioFile::getRange() -> Array

Thread safety: UNSAFE -- creates Array (heap allocation).
Returns the current sample range as a two-element array [start, end]. The range
defines which portion of the original audio file is active. Returns undefined
if no audio is loaded.

Pair with:
  setRange -- set the active range that getRange reports

Source:
  ScriptingApiObjects.cpp:1670  ScriptAudioFile::getRange()
    -> buffer->getCurrentRange() -> returns [start, end] as Array
