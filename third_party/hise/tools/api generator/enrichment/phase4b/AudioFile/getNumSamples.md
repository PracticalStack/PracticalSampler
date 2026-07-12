AudioFile::getNumSamples() -> Integer

Thread safety: SAFE
Returns the number of samples in the current range. After setRange(), returns
the sub-range size, not the total file length. Returns 0 if no audio is loaded.

Anti-patterns:
  - Do NOT use to get total file length after setRange() -- returns sub-range
    size. Use getTotalLengthInSamples() for the original file length.

Source:
  ScriptingApiObjects.cpp  ScriptAudioFile::getNumSamples()
    -> buffer->getBuffer().getNumSamples() (currentData, not originalBuffer)
