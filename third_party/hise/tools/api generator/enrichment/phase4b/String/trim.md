String::trim() -> String

Thread safety: UNSAFE -- allocates new string content.
Returns the string with leading and trailing whitespace removed.

Source:
  JavascriptEngineObjects.cpp:771-999  StringClass::trim()
    -> a.thisObject.toString().trim()
