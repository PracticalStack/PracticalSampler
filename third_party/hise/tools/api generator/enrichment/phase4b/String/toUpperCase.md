String::toUpperCase() -> String

Thread safety: UNSAFE -- allocates new string content.
Returns the string with all characters converted to uppercase.

Source:
  JavascriptEngineObjects.cpp:771-999  StringClass::toUpperCase()
    -> a.thisObject.toString().toUpperCase()
