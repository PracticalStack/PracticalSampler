String::endsWith(String suffix) -> Integer

Thread safety: WARNING -- string involvement, atomic ref-count operations.
Returns true if the string ends with the specified suffix. Case-sensitive.

Pair with:
  startsWith -- tests the beginning of the string instead

Source:
  JavascriptEngineObjects.cpp:771-999  StringClass::endsWith()
    -> a.thisObject.toString().endsWith(getString(a, 0))
