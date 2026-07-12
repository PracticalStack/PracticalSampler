AudioFile::getTotalLengthInSamples() -> Integer

Thread safety: SAFE
Returns the total length of the original audio file in samples, regardless of
the current range set by setRange. Returns undefined if no audio is loaded.

Pair with:
  getNumSamples -- returns current sub-range size (not total length)

Source:
  ScriptingApiObjects.cpp  ScriptAudioFile::getTotalLengthInSamples()
    -> buffer->getTotalRange().getEnd() (originalBuffer length)
