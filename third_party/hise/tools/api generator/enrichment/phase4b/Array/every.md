Array::every(Function callback) -> Integer

Thread safety: UNSAFE -- allocates scope objects for callback invocation.
Tests whether all elements pass the provided test function. Returns true if
the callback returns truthy for every element. Stops at first failure.
Callback signature: f(var element, int index, Array array)

Anti-patterns:
  - [BUG] Returns undefined on an empty array instead of true (JavaScript
    returns true for vacuous truth). Guard with a.isEmpty() || a.every(fn).
  - Undefined/void elements are silently skipped during iteration.

Pair with:
  some -- test if at least one element passes

Source:
  JavascriptEngineObjects.cpp  ArrayClass scoped function "every"
    -> callForEach() with ReturnFunction that breaks on first falsy result
