Array::contains(var value) -> Integer

Thread safety: WARNING -- linear scan O(n) over the array.
Returns true if the array contains the specified value, false otherwise.
Uses loose comparison (1 == 1.0).

Pair with:
  indexOf -- when you need the position, not just existence
  includes -- identical behavior (alias)

Source:
  JavascriptEngineObjects.cpp  ArrayClass::contains()
    -> array->contains(target) using var::operator==
