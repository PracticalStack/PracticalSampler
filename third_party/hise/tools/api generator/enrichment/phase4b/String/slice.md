String::slice(int start, int end) -> String

Thread safety: UNSAFE -- allocates new string content via substring extraction.
Returns a section of the string from start index to end index (exclusive). If
end is omitted, returns from start to end of string. Alias for substring() --
both call the same C++ implementation.

Source:
  JavascriptEngineObjects.cpp:771-999  StringClass (alias)
    -> same C++ function as substring()
    -> a.thisObject.toString().substring(start, end > 1 args ? end : 0x7fffffff)
