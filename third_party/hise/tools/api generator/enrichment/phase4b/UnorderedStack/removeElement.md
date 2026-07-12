UnorderedStack::removeElement(var index) -> undefined

Thread safety: SAFE
Removes the element at the specified index by swapping it with the last element
and decrementing the size. O(1) operation. Does NOT preserve element order.
Works in both float and event modes.

Anti-patterns:
  - Do NOT use index-based forward iteration while removing -- the swap-with-last
    behavior skips entries. Drain from index 0 in a while loop instead.

Source:
  CustomDataContainers.h  hise::UnorderedStack::removeElement()
    -> swap data[index] with data[position-1], decrement position
