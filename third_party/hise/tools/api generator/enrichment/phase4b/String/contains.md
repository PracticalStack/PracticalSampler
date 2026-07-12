String::contains(String searchString) -> Integer

Thread safety: WARNING -- string involvement, atomic ref-count operations.
Returns true if the string contains the specified substring. Case-sensitive.

Pair with:
  includes -- identical behavior (alias for contains)
  indexOf -- returns position instead of boolean

Source:
  JavascriptEngineObjects.cpp:771-999  StringClass::contains()
    -> a.thisObject.toString().contains(getString(a, 0))
