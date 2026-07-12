FixObjectArray::indexOf(ScriptObject obj) -> Integer

Thread safety: WARNING -- linear search O(n) using the factory's compare function
Returns the index of the first matching element, or -1 if not found. Argument
must be a FixObject from the same factory layout.

Dispatch/mechanics:
  Linear scan through size() elements
  compareFunction(item, obj) == 0 for equality test
  Default comparator: byte-level memcmp of entire element
  Property-based comparator: compares only specified properties

Pair with:
  contains -- when you only need existence, not position
  FixObjectFactory.setCompareFunction -- set before searching for field-specific matching

Anti-patterns:
  - Do NOT pass a plain JSON object -- silently returns -1 (dynamic_cast fails, no error)
  - Do NOT use default comparator for field-specific matching -- compares all bytes,
    so objects differing in any property (even unused ones) are considered unequal

Source:
  FixLayoutObjects.cpp:974  Array::indexOf()
    -> linear search with compareFunction(item, o) == 0
    -> returns -1 on dynamic_cast failure or no match
