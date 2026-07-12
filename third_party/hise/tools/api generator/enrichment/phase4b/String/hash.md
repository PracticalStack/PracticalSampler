String::hash() -> Integer

Thread safety: WARNING -- string involvement, atomic ref-count operations.
Returns a 64-bit hash code for the string. Two identical strings always produce
the same hash value.

Source:
  JavascriptEngineObjects.cpp:771-999  StringClass::hash()
    -> a.thisObject.toString().hashCode64()
