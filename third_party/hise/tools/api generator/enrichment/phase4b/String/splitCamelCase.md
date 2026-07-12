String::splitCamelCase() -> Array

Thread safety: UNSAFE -- array and string allocation.
Splits a camelCase or PascalCase string into an array of word tokens at
uppercase letter and digit boundaries. Consecutive uppercase letters form a
single token. Consecutive digits form a single token. Whitespace is stripped.

Dispatch/mechanics:
  Iterates chars: flushes token at uppercase/digit boundary
  "MyValue123Test" -> ["My", "Value", "123", "Test"]

Source:
  JavascriptEngineObjects.cpp:771-999  StringClass::splitCamelCase()
    -> custom char-by-char tokenizer with uppercase/digit boundary detection
