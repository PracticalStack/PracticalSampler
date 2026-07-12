Message::getProgramChangeNumber() -> Integer

Thread safety: SAFE
Returns the program change number (0-127) of a MIDI program change event. Returns -1
if the event is not a program change. Use isProgramChange() to check before calling.

Anti-patterns:
  - [BUG] Null-pointer guard checks messageHolder (mutable) instead of constMessageHolder,
    and the error message incorrectly says "setVelocity()" instead of
    "getProgramChangeNumber()". Fails with misleading error in read-only contexts.

Pair with:
  isProgramChange -- check event type before calling

Source:
  ScriptingApi.cpp  Message::getProgramChangeNumber()
    -> checks messageHolder != nullptr [BUG: should check constMessageHolder]
    -> constMessageHolder->isProgramChange() ? getNoteNumber() : -1
