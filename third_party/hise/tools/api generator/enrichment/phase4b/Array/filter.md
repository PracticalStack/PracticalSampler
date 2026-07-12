Array::filter(Function callback) -> Array

Thread safety: UNSAFE -- allocates scope objects and a new result array.
Creates a new array containing only elements for which the callback returns
truthy. Original array is not modified.
Callback signature: f(var element, int index, Array array)

Anti-patterns:
  - [BUG] Returns undefined on an empty array instead of an empty array.
    Check a.isEmpty() before calling if you need to chain methods on the result.
  - Undefined/void elements are silently skipped and never passed to the callback.

Pair with:
  map -- transform elements instead of selecting them
  find -- when you only need the first matching element

Source:
  JavascriptEngineObjects.cpp  ArrayClass scoped function "filter"
    -> callForEach() with ReturnFunction that appends truthy results to new array
