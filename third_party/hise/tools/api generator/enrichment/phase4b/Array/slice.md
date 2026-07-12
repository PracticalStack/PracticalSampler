Array::slice(int start, int end) -> Array

Thread safety: UNSAFE -- creates a new array with copied elements.
Returns a shallow copy of a portion of the array from start (inclusive) to
end (exclusive). Supports negative indices: -1 = last element, -2 =
second-to-last. If end is omitted, extracts through the end of the array.

Dispatch/mechanics:
  Negative indices resolved: start = max(0, size + start), end = max(0, size + end)
  Iterates from start to end, appends each element to new var via var::append()

Source:
  JavascriptEngineObjects.cpp  ArrayClass::slice()
    -> handles negative indices per JS spec
    -> result.append(array->getReference(i)) for range
