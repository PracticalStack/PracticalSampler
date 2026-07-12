Array::forEach(Function callback) -> undefined

Thread safety: UNSAFE -- allocates scope objects for callback invocation.
Executes the callback once for each element. Returns undefined.
Callback signature: f(var element, int index, Array array)

Anti-patterns:
  - Do NOT use forEach on the audio thread -- allocates scope objects
    internally. Use for (x in array) for allocation-free iteration.
  - Undefined/void elements are silently skipped.

Source:
  JavascriptEngineObjects.cpp  ArrayClass scoped function "forEach"
    -> callForEach() with ReturnFunction that always continues
