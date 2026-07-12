AudioSampleProcessor::getSampleRange() -> Array

Thread safety: UNSAFE -- allocates an Array for the return value.
Returns the active sample playback range as [start, end] in samples. Defines which
portion of the loaded audio file is used for playback. Returns undefined if no file loaded.
Dispatch/mechanics:
  getAudioFile(0)->getCurrentRange() -> returns [start, end] as Array
Pair with:
  setSampleRange -- set the active range
  getSampleStart -- get just the start position without Array allocation
Source:
  ScriptingApiObjects.cpp:4763+  getSampleRange() -> getCurrentRange() -> [getStart(), getEnd()]
