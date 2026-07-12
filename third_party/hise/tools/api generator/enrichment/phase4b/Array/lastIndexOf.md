Array::lastIndexOf(var value) -> Integer

Thread safety: WARNING -- linear scan O(n) from end of array.
Returns the index of the last occurrence of the specified value, or -1 if
not found. Searches backward. Always uses loose comparison (1 == 1.0).

Anti-patterns:
  - Unlike indexOf, there is no typeStrictness parameter. Comparison is
    always loose.

Pair with:
  indexOf -- search forward, with optional strict type matching

Source:
  JavascriptEngineObjects.cpp  ArrayClass::lastIndexOf()
    -> iterates backward using var::operator==
