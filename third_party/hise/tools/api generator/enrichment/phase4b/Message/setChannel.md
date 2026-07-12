Message::setChannel(Number newChannel) -> undefined

Thread safety: SAFE
Sets the MIDI channel (1-16) of the current event. Uses 1-based numbering matching MIDI
convention. Values outside 1-16 produce a script error in debug builds. Works on any
event type -- only requires a mutable callback context.

Anti-patterns:
  - Uses 1-based channels (1-16), not 0-based. Passing 0 triggers a script error in
    debug builds and wraps at uint8 level in release builds.

Source:
  ScriptingApi.cpp  Message::setChannel()
    -> ENABLE_SCRIPTING_SAFE_CHECKS: validates 1-16 range
    -> messageHolder->setChannel((uint8)newChannel)
