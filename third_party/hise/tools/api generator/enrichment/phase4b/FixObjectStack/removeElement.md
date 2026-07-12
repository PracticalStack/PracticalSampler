FixObjectStack::removeElement(Integer index) -> Integer

Thread safety: SAFE
Removes the element at index using swap-and-pop: the last used element is copied
into the slot at index, and the position pointer is decremented. The vacated last
slot is cleared to defaults. Returns 1 on success, 0 if index is out of range.

Dispatch/mechanics:
  position = jmax(0, position - 1)
  -> *items[index] = *items[position] (swap last into gap)
  -> items[position]->clear() (clear vacated slot)

Anti-patterns:
  - Do NOT assume element order is preserved -- swap-and-pop reorders elements.
    Call sort() after modifications if order matters.
  - Do NOT use simple forward iteration with removeElement() -- after removal,
    the swapped-in element at the current index is skipped. Use
    removeElement(i--) to re-check the current index.

Source:
  FixLayoutObjects.cpp:1345  Stack::removeElement()
    -> position = jmax(0, position - 1)
    -> *items[index] = *items[position]
    -> items[position]->clear()
