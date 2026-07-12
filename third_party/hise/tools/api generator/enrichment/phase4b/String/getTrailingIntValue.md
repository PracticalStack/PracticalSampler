String::getTrailingIntValue() -> Integer

Thread safety: WARNING -- string involvement, atomic ref-count operations.
Extracts and returns the integer at the end of the string. Returns 0 if the
string does not end with digits. Useful for extracting numeric suffixes from
component names (e.g., "Knob12" returns 12).

Pair with:
  getIntValue -- parses integer from the beginning of the string instead

Source:
  JavascriptEngineObjects.cpp:771-999  StringClass::getTrailingIntValue()
    -> a.thisObject.toString().getTrailingIntValue()
