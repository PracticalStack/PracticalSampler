Array::indexOf(var elementToLookFor, int startOffset, int typeStrictness) -> Integer

Thread safety: WARNING -- linear scan O(n) over the array.
Returns the index of the first occurrence of the specified value, or -1 if
not found. Searches forward from startOffset (default 0). typeStrictness
controls comparison: 0 (default) = loose (1 == 1.0), 1 = strict type+value.

Dispatch/mechanics:
  typeStrictness == 0: var::operator== (loose)
  typeStrictness == 1: var::equalsWithSameType (type AND value must match)

Pair with:
  lastIndexOf -- search backward from end
  contains -- when you only need existence, not position

Anti-patterns:
  - Default loose comparison means indexOf(1) matches both int 1 and double
    1.0. Use typeStrictness = 1 when you need to distinguish numeric types.

Source:
  JavascriptEngineObjects.cpp  ArrayClass::indexOf()
    -> iterates from startOffset, uses equalsWithSameType or operator==
