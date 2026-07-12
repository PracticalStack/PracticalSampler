AudioFile::getLoopRange(Integer subtractStart) -> Array

Thread safety: UNSAFE -- creates Array (heap allocation).
Returns the loop range as a two-element array [start, end] in sample positions.
If subtractStart is non-zero, positions are relative to the current range start.
If zero, positions are absolute within the original file. Returns undefined if
no audio is loaded.

Source:
  ScriptingApiObjects.cpp  ScriptAudioFile::getLoopRange()
    -> reads loopRange from MultiChannelAudioBuffer
    -> optionally subtracts bufferRange.getStart()
