String::includes(String searchString) -> Integer

Thread safety: WARNING -- string involvement, atomic ref-count operations.
Returns true if the string contains the specified substring. Case-sensitive.
Alias for contains() -- both call the same C++ implementation.

Pair with:
  contains -- identical behavior (canonical name)
  indexOf -- returns position instead of boolean

Source:
  JavascriptEngineObjects.cpp:771-999  StringClass (alias)
    -> same C++ function as contains()
    -> a.thisObject.toString().contains(getString(a, 0))
