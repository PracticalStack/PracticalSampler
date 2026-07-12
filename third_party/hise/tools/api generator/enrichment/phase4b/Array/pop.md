Array::pop() -> var

Thread safety: SAFE
Removes the last element from the array and returns it.
Returns undefined if the array is empty.

Pair with:
  push -- append to end (stack push/pop pattern)
  shift -- remove from front instead of back

Source:
  JavascriptEngineObjects.cpp  ArrayClass::pop()
    -> array->removeLast() equivalent
