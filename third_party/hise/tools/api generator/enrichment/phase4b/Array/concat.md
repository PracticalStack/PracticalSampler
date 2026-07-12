Array::concat(Array arrayToConcat) -> undefined

Thread safety: UNSAFE -- allocates memory for appended elements.
Appends all elements from one or more arrays to this array in-place.
Returns undefined. Accepts variadic array arguments. Non-array arguments
are silently ignored.

Dispatch/mechanics:
  Iterates each argument's .size() and calls array->insert(-1, element)
  for each. Non-array values have size() == 0, so they are skipped.

Anti-patterns:
  - Do NOT use var b = a.concat([4,5]) -- concat modifies in-place and returns
    undefined. Unlike JavaScript's Array.prototype.concat.
  - Do NOT pass non-array values to append individual elements -- they are
    silently ignored. Use push() for individual values.

Source:
  JavascriptEngineObjects.cpp  ArrayClass::concat()
    -> for each argument: newElements.size() iterations
    -> array->insert(-1, newElements[j])
