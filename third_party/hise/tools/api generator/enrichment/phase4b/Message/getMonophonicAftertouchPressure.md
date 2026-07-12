Message::getMonophonicAftertouchPressure() -> Integer

Thread safety: SAFE
Returns the pressure value of a monophonic (channel pressure) aftertouch event.
Only valid on a channel pressure event inside onController. Reports an error if
the event is not channel pressure or if called outside a MIDI callback.

Anti-patterns:
  - Both channel pressure and poly aftertouch use the same Aftertouch event type internally.
    Use isMonophonicAfterTouch() to check before calling, but note that it cannot
    reliably distinguish the two types -- check the note number field for disambiguation.

Pair with:
  isMonophonicAfterTouch -- check event type before calling
  setMonophonicAfterTouchPressure -- set the pressure value

Source:
  ScriptingApi.cpp  Message::getMonophonicAftertouchPressure()
    -> checks constMessageHolder->isChannelPressure()
    -> constMessageHolder->getAfterTouchValue()
