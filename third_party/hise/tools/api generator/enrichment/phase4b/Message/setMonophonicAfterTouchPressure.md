Message::setMonophonicAfterTouchPressure(Integer pressure) -> undefined

Thread safety: SAFE
Sets the pressure value of a monophonic (channel pressure) aftertouch event. Only valid
on an aftertouch event inside a mutable callback context. Value cast to uint8 (0-127).

Anti-patterns:
  - Both mono and poly aftertouch use the same Type::Aftertouch internally. The type check
    uses isChannelPressure() which returns true for ANY aftertouch event. Calling this on
    a polyphonic aftertouch event succeeds silently and overwrites the pressure value.

Pair with:
  getMonophonicAftertouchPressure -- read the pressure value
  isMonophonicAfterTouch -- check event type before calling

Source:
  ScriptingApi.cpp  Message::setMonophonicAfterTouchPressure()
    -> ENABLE_SCRIPTING_SAFE_CHECKS: checks messageHolder->isChannelPressure()
    -> messageHolder->setAfterTouchValue((uint8)pressure)
