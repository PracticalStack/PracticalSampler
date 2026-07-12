FixObjectArray::size() -> Integer

Thread safety: SAFE
Returns the fixed number of elements in the array. Always equals the length
constant set at creation time. For FixObjectArray, size() and length are
interchangeable. FixObjectStack overrides this to return current occupancy.

Source:
  FixLayoutObjects.cpp:1068  Array::size()
    -> returns (int)numElements
