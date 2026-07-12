FixObjectStack::set(ScriptObject obj) -> undefined

Thread safety: WARNING -- O(n) linear scan for existence check via indexOf.
Upsert operation: if a matching element exists (by compare function), replaces it
in place. If no match and stack is not full, inserts at end. The C++ method returns
bool but the scripting wrapper discards the return value.

Dispatch/mechanics:
  if empty: assign(position++, obj)
  else: indexOf(obj) for match
    -> if found: assign(idx, obj) (in-place replace)
    -> if not found and room: assign(position++, obj)
    -> if not found and full: returns false (discarded by wrapper)

Pair with:
  insert -- alternative that rejects duplicates instead of replacing
  contains -- check if element exists before deciding insert vs set

Anti-patterns:
  - [BUG] Scripting wrapper uses API_VOID_METHOD_WRAPPER_1, discarding the bool
    return. From script, set() always returns undefined -- impossible to detect
    when the stack is full and the insert was rejected.
  - [BUG] Shares insert's off-by-one: capacity check uses position < numElements-1,
    so effective capacity for new elements is length-1.

Source:
  FixLayoutObjects.cpp:1377  Stack::set()
    -> indexOf(obj) for existence check
    -> assign(index, obj) via Array::assign (AssignableObject)
    -> position++ for new inserts, clamped by isPositiveAndBelow(position, numElements-1)
