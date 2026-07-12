Array::map(Function callback) -> Array

Thread safety: UNSAFE -- allocates scope objects and a new result array.
Creates a new array with the results of calling the callback on every element.
Original array is not modified.
Callback signature: f(var element, int index, Array array)

Anti-patterns:
  - [BUG] Returns undefined on an empty array instead of an empty array.
    Check a.isEmpty() before calling if you need to chain methods on the result.
  - Undefined/void elements are silently skipped. An array [1, undefined, 3]
    produces a 2-element result, not 3.

Pair with:
  filter -- select elements instead of transforming them

Source:
  JavascriptEngineObjects.cpp  ArrayClass scoped function "map"
    -> callForEach() with ReturnFunction that appends callback return values
