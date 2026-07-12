AudioSampleProcessor::getTotalLengthInSamples() -> Integer

Thread safety: SAFE
Returns the total length of the loaded audio file in samples, regardless of the current
sample range. This is the full file length. Returns undefined if no file loaded.
Dispatch/mechanics:
  getAudioFile(0)->getTotalRange().getEnd()
Pair with:
  getSampleLength -- get the active range length instead
  setSampleRange -- use the total length as the end value for full-file playback
Source:
  ScriptingApiObjects.cpp:4763+  getTotalLengthInSamples() -> getTotalRange().getEnd()
