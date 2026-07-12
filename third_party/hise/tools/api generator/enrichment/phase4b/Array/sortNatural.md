Array::sortNatural() -> Array

Thread safety: UNSAFE -- converts all elements to strings internally, which
involves allocation.
Sorts the array in-place using natural string comparison with embedded number
awareness ("item2" sorts before "item10"). Returns the array itself (enables
chaining). HISE-specific method.

Dispatch/mechanics:
  std::sort with juce::String::compareNatural
  All elements converted to String via implicit var -> String conversion

Anti-patterns:
  - Converts ALL elements to strings for comparison regardless of actual type.
    A mixed-type array will be sorted by string representation.

Pair with:
  sort -- numeric sort or custom comparator sort

Source:
  JavascriptEngineObjects.cpp  ArrayClass::sortNatural()
    -> std::sort with String::compareNatural lambda
