String::startsWith(String prefix) -> Integer

Thread safety: WARNING -- string involvement, atomic ref-count operations.
Returns true if the string starts with the specified prefix. Case-sensitive.

Pair with:
  endsWith -- tests the end of the string instead

Source:
  JavascriptEngineObjects.cpp:771-999  StringClass::startsWith()
    -> a.thisObject.toString().startsWith(getString(a, 0))
