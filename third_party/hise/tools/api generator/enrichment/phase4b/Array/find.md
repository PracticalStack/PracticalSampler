Array::find(Function callback) -> var

Thread safety: UNSAFE -- allocates scope objects for callback invocation.
Returns the first element for which the callback returns truthy.
Returns undefined if no match is found.
Callback signature: f(var element, int index, Array array)

Pair with:
  findIndex -- when you need the position, not the value
  filter -- when you need all matching elements

Source:
  JavascriptEngineObjects.cpp  ArrayClass scoped function "find"
    -> callForEach() with ReturnFunction that breaks on first truthy result
