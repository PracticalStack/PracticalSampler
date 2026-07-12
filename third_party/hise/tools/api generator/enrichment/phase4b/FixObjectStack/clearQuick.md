FixObjectStack::clearQuick() -> undefined

Thread safety: SAFE
Resets the position pointer to zero without modifying element data. Elements
remain in memory but are no longer accessible through size(), indexOf(), or
contains(). Subsequent inserts overwrite the old data.

Anti-patterns:
  - Do NOT assume toBase64() reflects only used data after clearQuick() --
    toBase64() serializes the full memory block including logically removed slots

Source:
  FixLayoutObjects.cpp:1367  Stack::clearQuick()
    -> position = 0
