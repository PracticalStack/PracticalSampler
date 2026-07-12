FixObjectStack::fill(ScriptObject obj) -> undefined

Thread safety: SAFE
Copies data from obj into ALL allocated slots (full capacity). Does not update
the position pointer -- size() remains unchanged after fill().

Anti-patterns:
  - Do NOT expect fill() to update size() -- position stays at its previous value
    even though all slots now contain valid data

Pair with:
  clear -- to reset all slots to defaults (fill writes custom data, clear writes defaults)

Source:
  FixLayoutObjects.cpp:955  Array::fill()
    -> iterates all items (numElements), copies ObjectReference data into each slot
    -> does not modify position
