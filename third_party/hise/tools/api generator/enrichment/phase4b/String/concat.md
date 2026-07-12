String::concat(String arg1, ...) -> String

Thread safety: UNSAFE -- allocates new string content.
Appends one or more string arguments and returns the combined result. Accepts
variable number of arguments -- all are converted to strings and concatenated.

Source:
  JavascriptEngineObjects.cpp:771-999  StringClass::concat()
    -> iterates a.numArguments, appends each via getString(a, i)
