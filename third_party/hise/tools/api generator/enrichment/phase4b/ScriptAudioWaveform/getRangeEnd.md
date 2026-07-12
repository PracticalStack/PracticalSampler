ScriptAudioWaveform::getRangeEnd() -> Integer

Thread safety: SAFE
Returns the end position (in samples) of the currently selected sample range.
Returns 0 if no audio data is loaded.

Pair with:
  getRangeStart -- together define the selected sample range

Source:
  ScriptingApiContent.cpp  ScriptAudioWaveform::getRangeEnd()
    -> getCachedAudioFile()->getCurrentRange().getEnd()
