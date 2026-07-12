AudioFile::getSampleRate() -> Double

Thread safety: SAFE
Returns the sample rate of the loaded audio file in Hz. Returns 0.0 if no
audio is loaded.

Source:
  ScriptingApiObjects.cpp  ScriptAudioFile::getSampleRate()
    -> buffer->sampleRate
