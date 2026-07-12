String::capitalize() -> String

Thread safety: UNSAFE -- allocates new string content.
Converts the first letter of each word to uppercase (title case). Splits on
spaces, uppercases first character of each token, rejoins with spaces.

Source:
  JavascriptEngineObjects.cpp:771-999  StringClass::capitalize()
    -> splits by space, uppercases first char of each token, rejoins
