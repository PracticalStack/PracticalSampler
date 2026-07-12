Array::findIndex(Function callback) -> Integer

Thread safety: UNSAFE -- allocates scope objects for callback invocation.
Returns the index of the first element for which the callback returns truthy.
Returns undefined if no match is found.
Callback signature: f(var element, int index, Array array)

Anti-patterns:
  - [BUG] Returns undefined when no element matches, not -1 as in JavaScript
    and as indexOf does. Code checking findIndex(fn) == -1 will never match.
    Use isDefined(result) to detect not-found.

Pair with:
  find -- when you need the value, not the position
  indexOf -- for value-based lookup without a callback

Source:
  JavascriptEngineObjects.cpp  ArrayClass scoped function "findIndex"
    -> callForEach() with ReturnFunction that breaks on first truthy, returns index
