AudioSampleProcessor::getSampleStart() -> Integer

Thread safety: SAFE
Returns the start position of the current sample range in samples. Equivalent to
getSampleRange()[0] but avoids the Array allocation. Returns 0 if no file loaded.
Pair with:
  getSampleRange -- get both start and end (allocates Array)
  setSampleRange -- set the active range
Source:
  ScriptingApiObjects.cpp:4763+  getSampleStart() -> getAudioFile(0)->getCurrentRange().getStart()
