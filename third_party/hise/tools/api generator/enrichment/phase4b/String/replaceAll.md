String::replaceAll(String search, String replacement) -> String

Thread safety: UNSAFE -- allocates new string content.
Replaces ALL occurrences of the search string with the replacement string.
Alias for replace() -- both call the same C++ implementation. Exists for
JavaScript API familiarity.

Source:
  JavascriptEngineObjects.cpp:771-999  StringClass (alias)
    -> same C++ function as replace()
    -> a.thisObject.toString().replace(getString(a, 0), getString(a, 1))
