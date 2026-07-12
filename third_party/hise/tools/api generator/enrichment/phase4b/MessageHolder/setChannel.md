MessageHolder::setChannel(Number newChannel) -> undefined

Thread safety: SAFE
Sets the MIDI channel. Stored as uint8 (0-255), supporting HISE's extended range
beyond standard MIDI 1-16. No range validation -- cast directly to uint8.

Anti-patterns:
  - Do NOT leave channel at 0 for standard MIDI use -- MIDI channels start at 1.
    A newly created MessageHolder defaults to channel 0.

Pair with:
  getChannel -- read the channel

Source:
  ScriptingApiObjects.cpp  ScriptingMessageHolder::setChannel()
    -> e.setChannel((uint8)newChannel)
