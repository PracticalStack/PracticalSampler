AudioFile::getCurrentlyDisplayedIndex() -> Double

Thread safety: SAFE
Returns the current display position index as a floating-point value. This is
the playback position reported by the audio processor, updated asynchronously.
Useful for syncing UI elements with playback progress.

Pair with:
  setDisplayCallback -- register a callback to react to position changes

Source:
  ScriptingApiObjects.h:1076  ScriptComplexDataReferenceBase::getCurrentDisplayIndexBase()
    -> reads cached display position from ComplexDataUIUpdaterBase
