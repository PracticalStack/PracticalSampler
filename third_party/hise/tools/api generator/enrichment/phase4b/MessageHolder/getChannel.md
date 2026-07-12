MessageHolder::getChannel() -> Integer

Thread safety: SAFE
Returns the MIDI channel. HISE supports 0-255 internally (uint8), extending beyond
standard MIDI 1-16. A newly created MessageHolder has channel 0.

Pair with:
  setChannel -- set the MIDI channel

Source:
  ScriptingApiObjects.cpp  ScriptingMessageHolder::getChannel()
    -> (int)e.getChannel()
