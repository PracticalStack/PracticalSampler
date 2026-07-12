String::split(String separator) -> Array

Thread safety: UNSAFE -- array and string allocation.
Splits the string by a separator and returns an array of substrings. If the
separator is empty, splits into individual characters.

Anti-patterns:
  - Only the FIRST CHARACTER of the separator is used as the delimiter.
    Multi-character separators like "::" or ", " are silently truncated to
    their first character. This differs from JavaScript.

Dispatch/mechanics:
  sep.substring(0, 1) -> StringArray::addTokens(str, firstChar, "")
  Empty separator: iterates CharPointer, adds each char individually

Source:
  JavascriptEngineObjects.cpp:771-999  StringClass::split()
    -> addTokens(str, sep.substring(0, 1), "")
