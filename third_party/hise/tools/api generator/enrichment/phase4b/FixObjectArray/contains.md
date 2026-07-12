FixObjectArray::contains(ScriptObject obj) -> Integer

Thread safety: WARNING -- linear search O(n) using the factory's compare function
Returns 1 if the array contains a matching element, 0 otherwise.
Argument must be a FixObject from the same factory layout.

Dispatch/mechanics:
  contains(obj) -> indexOf(obj) != -1
    -> linear scan with compareFunction(item, obj) == 0

Pair with:
  indexOf -- when you need the position, not just existence
  FixObjectFactory.setCompareFunction -- controls what "matching" means

Anti-patterns:
  - Do NOT pass a plain JSON object -- silently returns 0 (dynamic_cast fails, no error)
  - Do NOT rely on default comparator for field-specific matching -- default is byte-level
    memcmp of entire element, not property-based

Source:
  FixLayoutObjects.cpp:1119  Array::contains()
    -> indexOf(obj) != -1
  FixLayoutObjects.cpp:974  Array::indexOf()
    -> linear search with compareFunction
