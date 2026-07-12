UnorderedStack::clear() -> Integer

Thread safety: SAFE
Removes all elements. Returns true if the stack was non-empty before clearing,
false if already empty. Works in both float and event modes.

Dispatch/mechanics:
  Calls data.clear() or eventData.clear() (memset to zero + reset position)
  Float mode: also calls updateElementBuffer() to reset the buffer view size

Pair with:
  isEmpty -- check before clearing if you need conditional logic
  size -- returns 0 after clear

Source:
  ScriptingApiObjects.cpp  clear()
    -> hise::UnorderedStack::clear() -- memset + position = 0
    -> returns previous !isEmpty() state
