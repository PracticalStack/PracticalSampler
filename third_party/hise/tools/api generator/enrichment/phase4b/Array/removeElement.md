Array::removeElement(int index) -> undefined

Thread safety: WARNING -- element shifting O(n) after removed index.
Removes the element at the specified index. Elements after the removed index
shift left. HiseScript equivalent of JavaScript's splice(index, 1).
Out-of-range indices are silently ignored.

Anti-patterns:
  - Do NOT use removeElement(i) in a forward loop without adjusting the
    index -- removing shifts elements left, so the next element moves to the
    current index and gets skipped. Use removeElement(i--) or iterate backward.

Pair with:
  remove -- remove by value instead of by index

Source:
  JavascriptEngineObjects.cpp  ArrayClass::removeElement()
    -> array->remove(index)
