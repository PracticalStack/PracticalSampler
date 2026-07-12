FixObjectArray::clear() -> undefined

Thread safety: SAFE
Resets all elements to their default values (0/0.0/false per the factory prototype).
Array size remains unchanged -- all length elements stay valid and iterable.

Dispatch/mechanics:
  clear() -> fill(var()) -> ObjectReference::clear() on every slot
    -> each member reset to prototype default via MemoryLayoutItem

Pair with:
  fill -- clear() is equivalent to fill(non-FixObject), but fill(FixObject) broadcasts a template

Anti-patterns:
  - Do NOT assume clear() changes the iteration range -- unlike FixObjectStack.clear(),
    this does not affect size() or for-in loop length

Source:
  FixLayoutObjects.cpp:969  Array::clear()
    -> fill(var()) -> iterates items, calls ObjectReference::clear()
