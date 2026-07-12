Array::sort(Function comparator) -> Array

Thread safety: WARNING -- without a comparator, sorts in-place with no
allocation (safe for numeric data). With a comparator function, allocates
scope objects -- not audio-thread safe.
Sorts the array in-place. Without a comparator, uses VariantComparator
(numeric sort). With a comparator, uses std::stable_sort. Returns the array
itself (enables chaining).
Callback signature: comparator(var a, var b) -- return negative if a < b, 0 if equal, positive if a > b

Dispatch/mechanics:
  No comparator: juce::Array::sort(VariantComparator)
    -> numeric types compared by value, mixed int/double promotes to double
    -> strings all compare as equal (remain unsorted)
    -> arrays/objects throw runtime exception
  With comparator: std::stable_sort with user function
    -> preserves relative order of equal elements

Anti-patterns:
  - Do NOT use sort() without a comparator on string arrays -- strings all
    compare as equal and remain unsorted. Use sortNatural() or provide a
    custom comparator.
  - Do NOT pass arrays or objects as elements without a comparator -- throws
    a runtime exception.

Pair with:
  sortNatural -- string-aware natural sort

Source:
  JavascriptEngineObjects.cpp  ArrayClass scoped function "sort"
  JavascriptApiClass.h:17-45  VariantComparator
