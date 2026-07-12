Message::setStartOffset(Number newStartOffset) -> undefined

Thread safety: SAFE
Sets the sample start offset on the current event. Tells the sound generator to skip
ahead by this many samples at voice start -- does NOT delay event processing (use
delayEvent for that). Stored as uint16, max 65535 (~1.36s at 48kHz). Values above
65535 produce a script error.

Anti-patterns:
  - [BUG] Null check tests constMessageHolder (const) but the write uses messageHolder
    (mutable). In read-only contexts, the null check passes then dereferences null
    messageHolder -- undefined behavior.
  - [BUG] Error message says "Max start offset is 65536 (2^16)" but actual max is 65535.

Pair with:
  delayEvent -- delays event processing (different purpose)

Source:
  ScriptingApi.cpp  Message::setStartOffset()
    -> checks constMessageHolder != nullptr [BUG: should check messageHolder]
    -> checks newStartOffset > UINT16_MAX -> script error
    -> messageHolder->setStartOffset((uint16)newStartOffset)
