Engine::sortWithFunction(var arrayToSort, var sortFunction) -> Integer

Thread safety: UNSAFE -- calls script function repeatedly during sort
Sorts array in-place using custom comparator. Returns negative (a<b), zero, or positive (a>b).
Returns false silently if arguments invalid.
Callback signature: sortFunction(var a, var b)
Source:
  ScriptingApi.cpp  Engine::sortWithFunction()
    -> JUCE Array sort with callExternalFunctionRaw comparator
