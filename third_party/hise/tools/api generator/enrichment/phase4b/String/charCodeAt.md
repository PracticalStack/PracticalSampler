String::charCodeAt(int position) -> Integer

Thread safety: WARNING -- string involvement, atomic ref-count operations.
Returns the Unicode code point (integer) of the character at the specified
position. Returns 0 if position is out of bounds.

Pair with:
  charAt -- returns the character as a string instead of its code point

Source:
  JavascriptEngineObjects.cpp:771-999  StringClass::charCodeAt()
    -> (int)a.thisObject.toString()[getInt(a, 0)]
