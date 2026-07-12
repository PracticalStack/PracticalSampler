MessageHolder::clone() -> ScriptObject

Thread safety: UNSAFE -- allocates a new MessageHolder object on the heap.
Creates and returns an independent copy. Modifying the clone does not affect the original.

Dispatch/mechanics:
  new ScriptingMessageHolder(getScriptProcessor()) -> setMessage(e)
  Returns a new reference-counted object with a full HiseEvent value copy.

Pair with:
  Engine.createMessageHolder -- alternative: create empty then configure
  Message.store -- alternative: capture live event into existing holder

Source:
  ScriptingApiObjects.cpp:5620  ScriptingMessageHolder::clone()
    -> new ScriptingMessageHolder() -> setMessage(e) -> return var(no)
