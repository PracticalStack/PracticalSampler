String::getIntValue() -> Integer

Thread safety: WARNING -- string involvement, atomic ref-count operations.
Parses the string as a 64-bit integer from the beginning. Returns 0 if the
string does not start with a numeric character.

Pair with:
  getTrailingIntValue -- parses integer from the end of the string instead

Source:
  JavascriptEngineObjects.cpp:771-999  StringClass::getIntValue()
    -> a.thisObject.toString().getLargeIntValue()
