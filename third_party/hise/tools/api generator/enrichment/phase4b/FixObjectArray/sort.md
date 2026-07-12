FixObjectArray::sort() -> undefined

Thread safety: WARNING -- O(n log n) std::sort; if a custom JavaScript compare function is set, each comparison invokes a script callback
Sorts the array elements in place using the factory's compare function.
Only meaningful after setting a property-based or custom compare function.

Dispatch/mechanics:
  Creates local Sorter struct delegating to compareFunction
  std::sort via SortFunctionConverter adapter
  Sorts first size() elements (matters for FixObjectStack subclass)

Pair with:
  FixObjectFactory.setCompareFunction -- must configure before sorting

Anti-patterns:
  - Do NOT call sort() without setting a compare function first -- default comparator
    orders by pointer address, producing arbitrary results with no warning

Source:
  FixLayoutObjects.cpp:1073  Array::sort()
    -> Sorter{compareFunction} -> std::sort via SortFunctionConverter
