String::substring(int start, int end) -> String

Thread safety: UNSAFE -- allocates new string content.
Returns a section of the string from start index to end index (exclusive). If
end is omitted, returns from start to end of string.

Dispatch/mechanics:
  Uses 0x7fffffff as end index when only one argument provided (effectively
  "to end of string")

Source:
  JavascriptEngineObjects.cpp:771-999  StringClass::substring()
    -> a.thisObject.toString().substring(getInt(a, 0),
         a.numArguments > 1 ? getInt(a, 1) : 0x7fffffff)
