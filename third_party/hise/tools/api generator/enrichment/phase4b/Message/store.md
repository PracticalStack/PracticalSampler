Message::store(ScriptObject messageEventHolder) -> undefined

Thread safety: SAFE
Copies the current MIDI event into a MessageHolder object, allowing event data to persist
beyond the callback scope. The copy is a full 16-byte HiseEvent including all fields.
Uses the const pointer internally, so works in both mutable and read-only contexts.

Required setup:
  const var holder = Engine.createMessageHolder();

Pair with:
  Engine.createMessageHolder -- create the holder object
  Synth.addMessageFromHolder -- replay a stored event

Anti-patterns:
  - [BUG] If the argument is not a valid MessageHolder, the method silently does nothing
    (dynamic_cast fails and falls through). No error is reported. Only pass objects from
    Engine.createMessageHolder().

Source:
  ScriptingApi.cpp  Message::store()
    -> checks constMessageHolder != nullptr
    -> dynamic_cast to ScriptingMessageHolder
    -> holder->setMessage(*constMessageHolder) -- full HiseEvent copy
