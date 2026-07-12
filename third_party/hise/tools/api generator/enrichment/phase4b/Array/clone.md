Array::clone() -> Array

Thread safety: UNSAFE -- allocates a new array and recursively clones all elements.
Creates a deep copy of the array. Nested arrays and objects are recursively
cloned. The returned array is fully independent of the original.

Dispatch/mechanics:
  Inherited from ObjectClass, not ArrayClass.
  a.thisObject.clone() -> juce::var::clone()
    -> creates new Array<var>, recursively clones each element

Anti-patterns:
  - Do NOT use assignment (var b = a) when you need an independent copy --
    assignment creates a reference. Both variables point to the same array.

Source:
  JavascriptEngineObjects.cpp  ObjectClass::cloneFn()
    -> juce::var::clone() (deep copy for arrays)
