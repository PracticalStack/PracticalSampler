String::charAt(int position) -> String

Thread safety: UNSAFE -- allocates new string content via substring extraction.
Returns the character at the specified position as a single-character string.
Returns empty string if position is out of bounds.

Source:
  JavascriptEngineObjects.cpp:771-999  StringClass::charAt()
    -> a.thisObject.toString().substring(p, p + 1)
