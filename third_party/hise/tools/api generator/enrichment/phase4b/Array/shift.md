Array::shift() -> var

Thread safety: WARNING -- shifts all remaining elements left O(n).
Removes the first element from the array and returns it. All remaining
elements shift left by one index. Returns undefined if the array is empty.

Pair with:
  pop -- remove from end instead of front

Source:
  JavascriptEngineObjects.cpp  ArrayClass::shift()
    -> array->remove(0)
