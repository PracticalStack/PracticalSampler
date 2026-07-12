Array::some(Function callback) -> Integer

Thread safety: UNSAFE -- allocates scope objects for callback invocation.
Tests whether at least one element passes the provided test function. Returns
true if any callback returns truthy. Stops at the first passing element.
Callback signature: f(var element, int index, Array array)

Anti-patterns:
  - [BUG] Returns undefined on an empty array instead of false (JavaScript
    returns false). Guard with !a.isEmpty() && a.some(fn).
  - Undefined/void elements are silently skipped during iteration.

Pair with:
  every -- test if ALL elements pass

Source:
  JavascriptEngineObjects.cpp  ArrayClass scoped function "some"
    -> callForEach() with ReturnFunction that breaks on first truthy result
