FixObjectStack::contains(ScriptObject obj) -> Integer

Thread safety: WARNING -- O(n) linear scan over used elements via the compare function.
Returns 1 if the stack contains an element matching obj according to the factory's
compare function, 0 otherwise. Only searches the used portion (0 to size()-1).

Anti-patterns:
  - Do NOT pass non-FixObject arguments -- silently returns 0 (no error reported)

Source:
  FixLayoutObjects.cpp:1119  Array::contains() delegates to indexOf(obj) != -1
    -> Array::indexOf() uses virtual size() = position for search range
    -> compareFunction(item, obj) == 0 for equality
