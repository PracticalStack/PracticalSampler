FixObjectStack::remove(ScriptObject obj) -> Integer

Thread safety: WARNING -- O(n) linear scan for element lookup via indexOf.
Finds and removes the first element matching obj using the factory's compare
function. Uses swap-and-pop: the last used element moves into the gap. Returns
1 if found and removed, 0 if not found.

Dispatch/mechanics:
  indexOf(obj) to find match -> removeElement(idx) for swap-and-pop removal

Pair with:
  insert -- to add elements that can later be removed
  indexOf -- to check existence before removing
  removeElement -- direct index-based removal (no search)

Anti-patterns:
  - Do NOT assume element order is preserved after remove -- swap-and-pop moves
    the last element into the gap. Call sort() after removal if order matters.

Source:
  FixLayoutObjects.cpp:1335  Stack::remove()
    -> indexOf(obj) using compare function
    -> delegates to removeElement(idx) for swap-and-pop
