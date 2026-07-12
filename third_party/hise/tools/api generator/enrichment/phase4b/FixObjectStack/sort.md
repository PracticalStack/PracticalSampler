FixObjectStack::sort() -> undefined

Thread safety: WARNING -- O(n log n) comparison sort over used elements.
Sorts the used portion of the stack (indices 0 to size()-1) using the factory's
compare function. Unused slots beyond the position pointer are not affected.

Pair with:
  remove/removeElement -- sort after swap-and-pop removal to restore order

Source:
  FixLayoutObjects.cpp:1073  Array::sort()
    -> std::sort from items.begin() to items.begin() + size()
    -> uses virtual size() = position, so only sorts used elements
