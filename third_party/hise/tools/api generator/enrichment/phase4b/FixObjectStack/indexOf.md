FixObjectStack::indexOf(ScriptObject obj) -> Integer

Thread safety: WARNING -- O(n) linear scan over used elements via the compare function.
Returns the index of the first element matching obj according to the factory's
compare function, or -1 if not found. Only searches the used portion (0 to size()-1).

Anti-patterns:
  - Do NOT pass non-FixObject arguments -- silently returns -1 (no error reported)

Pair with:
  removeElement -- use indexOf() result to remove by index
  contains -- boolean wrapper around indexOf

Source:
  FixLayoutObjects.cpp:974  Array::indexOf()
    -> uses virtual size() = position for search range
    -> compareFunction(item, obj) == 0 for equality check
