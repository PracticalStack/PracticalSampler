Array::reverse() -> undefined

Thread safety: UNSAFE -- creates a temporary array copy internally, allocating
memory.
Reverses the order of elements in the array in-place.

Anti-patterns:
  - Returns undefined, not the array itself. Unlike sort() and sortNatural()
    which return the array for chaining.

Source:
  JavascriptEngineObjects.cpp  ArrayClass::reverse()
    -> creates reversed copy, swaps with original
