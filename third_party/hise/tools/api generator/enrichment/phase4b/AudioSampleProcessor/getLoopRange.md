AudioSampleProcessor::getLoopRange(Number subtractStart) -> Array

Thread safety: UNSAFE -- allocates an Array for the return value.
Returns the loop range as [start, end] in samples. When subtractStart is true, positions
are relative to the current sample range start. When false, positions are absolute.
Dispatch/mechanics:
  ProcessorWithExternalData->getAudioFile(0)->getLoopRange(subtractStart)
Pair with:
  getSampleRange -- get the active playback range for context
  getSampleStart -- the value subtracted when subtractStart is true
Source:
  ScriptingApiObjects.cpp:4763+  getLoopRange() -> getAudioFile(0)->getLoopRange(subtractStart)
