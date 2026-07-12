FixObjectStack::clear() -> undefined

Thread safety: SAFE
Resets all allocated slots to default values and sets position pointer to zero.
Unlike clearQuick(), writes defaults into every slot including those beyond the
current position.

Dispatch/mechanics:
  Iterates all items (full capacity) calling ObjectReference::clear() on each
  -> then calls clearQuick() to reset position to 0

Pair with:
  clearQuick -- lightweight alternative when data will be overwritten anyway

Anti-patterns:
  - Do NOT use clear() in a tight loop when the stack will be repopulated
    immediately -- clearQuick() is faster (O(1) vs O(n) over full capacity)

Source:
  FixLayoutObjects.cpp:1359  Stack::clear()
    -> iterates all items: ObjectReference::clear()
    -> clearQuick() sets position = 0
