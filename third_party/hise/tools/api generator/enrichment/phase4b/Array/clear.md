Array::clear() -> undefined

Thread safety: SAFE
Removes all elements from the array without deallocating internal storage.
The array becomes empty but retains its allocated capacity.

Dispatch/mechanics:
  clearQuick() on the underlying juce::Array<var> -- sets size to 0, keeps buffer

Pair with:
  isEmpty -- check if already empty before clearing

Anti-patterns:
  - Do NOT assume clear() releases memory -- to both clear and deallocate,
    reassign to a new empty array: a = [];

Source:
  JavascriptEngineObjects.cpp  ArrayClass::clear()
    -> array->clearQuick()
