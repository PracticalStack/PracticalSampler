Engine::setKeyColour(int keyNumber, int colourAsHex) -> undefined

Thread safety: UNSAFE -- writes to internal array, broadcasts change message
Sets colour of a single key on the on-screen MIDI keyboard. Use 0xAARRGGBB format.
Pass 0x00000000 to clear a key's custom colour.
Anti-patterns:
  - Always set all 128 keys (including unmapped keys to dim colour) to prevent
    stale colours from a previous state
Pair with:
  setLowestKeyToDisplay -- scroll keyboard view
Source:
  ScriptingApi.cpp  Engine::setKeyColour()
    -> CustomKeyboardState::setColourForSingleKey() -> sendChangeMessage()
