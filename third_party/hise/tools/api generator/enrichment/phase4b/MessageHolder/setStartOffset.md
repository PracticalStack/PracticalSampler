MessageHolder::setStartOffset(Number offset) -> undefined

Thread safety: SAFE
Sets the sample start offset for voice playback. Cast to uint16 (0-65535). Unlike
the timestamp (which positions the event in the audio buffer), the start offset
tells the sound generator to skip samples when the voice starts -- used for
sample-accurate voice positioning within a longer sample.

Anti-patterns:
  - Negative values or values above 65535 silently wrap via uint16 cast.
  - There is no getStartOffset() on MessageHolder -- the value is write-only
    from the scripting side.

Pair with:
  setTimestamp -- positions the event in time (different purpose)
  Synth.addMessageFromHolder -- the sound generator reads the start offset internally

Source:
  ScriptingApiObjects.cpp  ScriptingMessageHolder::setStartOffset()
    -> e.setStartOffset((uint16)offset)
