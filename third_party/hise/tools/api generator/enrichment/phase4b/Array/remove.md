Array::remove(var value) -> undefined

Thread safety: WARNING -- linear scan and element shifting O(n).
Removes ALL instances of the specified value from the array. Uses loose
comparison. Does nothing if the value is not found.

Anti-patterns:
  - Removes ALL matching instances, not just the first. To remove at a
    specific index, use removeElement().

Pair with:
  removeElement -- remove by index instead of by value

Source:
  JavascriptEngineObjects.cpp  ArrayClass::remove()
    -> iterates and removes all matching elements
