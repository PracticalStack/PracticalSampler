AudioSampleProcessor::setSampleRange(Number start, Number end) -> undefined

Thread safety: UNSAFE -- sends a range change notification to the audio file buffer.
Sets the active sample playback range in samples. Defines which portion of the loaded
audio file is used for playback.
Dispatch/mechanics:
  getAudioFile(0)->setRange(Range<int>(start, end))
    -> sends range change notification to MultiChannelAudioBuffer
Pair with:
  getSampleRange -- query the current range
  getTotalLengthInSamples -- get the full file length for full-range playback
Source:
  ScriptingApiObjects.cpp:4763+  setSampleRange() -> getAudioFile(0)->setRange(Range<int>(start, end))
