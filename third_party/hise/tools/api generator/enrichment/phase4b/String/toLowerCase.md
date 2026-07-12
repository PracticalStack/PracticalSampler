String::toLowerCase() -> String

Thread safety: UNSAFE -- allocates new string content.
Returns the string with all characters converted to lowercase.

Source:
  JavascriptEngineObjects.cpp:771-999  StringClass::toLowerCase()
    -> a.thisObject.toString().toLowerCase()
