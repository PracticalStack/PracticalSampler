ScriptAudioWaveform::getRangeStart() -> Integer

Thread safety: SAFE
Returns the start position (in samples) of the currently selected sample range.
Returns 0 if no audio data is loaded.

Pair with:
  getRangeEnd -- together define the selected sample range

Source:
  ScriptingApiContent.cpp  ScriptAudioWaveform::getRangeStart()
    -> getCachedAudioFile()->getCurrentRange().getStart()
