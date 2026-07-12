Message::getChannel() -> Integer

Thread safety: SAFE
Returns the MIDI channel (1-16) of the current event. Works on any event type --
does not require a specific event type, only an active MIDI callback context.

Source:
  ScriptingApi.cpp  Message::getChannel()
    -> constMessageHolder->getChannel()
