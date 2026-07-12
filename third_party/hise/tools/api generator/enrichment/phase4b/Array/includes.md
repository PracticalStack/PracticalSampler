Array::includes(var value) -> Integer

Thread safety: WARNING -- linear scan O(n) over the array.
Alias for contains(). Returns true if the array contains the specified value.
Uses loose comparison (1 == 1.0). Provided for JavaScript API compatibility.

Pair with:
  contains -- identical behavior (native name)

Source:
  JavascriptEngineObjects.cpp  ArrayClass constructor
    -> setMethod("includes", contains) -- same function pointer as contains
