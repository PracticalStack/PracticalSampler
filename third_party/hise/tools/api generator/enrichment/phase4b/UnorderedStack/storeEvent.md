UnorderedStack::storeEvent(var index, var holder) -> undefined

Thread safety: SAFE
Event-mode only. Copies the event at the specified index into the provided
MessageHolder. The stack is not modified. Reports a script error if the index
is out of bounds or if called on a float-mode stack.

Pair with:
  removeElement -- use after storeEvent to drain events in a while loop:
    while(!es.isEmpty()) { es.storeEvent(0, holder); es.removeElement(0); }
  removeIfEqual -- alternative that finds by compare function and pops in one call

Anti-patterns:
  - Do NOT call on a float-mode stack -- reports a script error

Source:
  ScriptingApiObjects.cpp  storeEvent()
    -> isPositiveAndBelow(index, size()) bounds check
    -> copies eventData[index] into ScriptingMessageHolder
