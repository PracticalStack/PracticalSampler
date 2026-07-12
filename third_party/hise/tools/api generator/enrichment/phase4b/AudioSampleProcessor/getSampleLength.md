AudioSampleProcessor::getSampleLength() -> Integer

Thread safety: SAFE
Returns the length of the current sample range in samples (end - start), not the total
file length. Returns 0 if no file is loaded or the handle is invalid.
Dispatch/mechanics:
  getAudioFile(0)->getCurrentRange().getLength()
Pair with:
  getSampleRange -- get the start and end positions
  getTotalLengthInSamples -- get the full file length instead
Source:
  ScriptingApiObjects.cpp:4763+  getSampleLength() -> getCurrentRange().getLength()
