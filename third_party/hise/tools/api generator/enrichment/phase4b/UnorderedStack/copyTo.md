UnorderedStack::copyTo(var target) -> Integer

Thread safety: WARNING -- Array target path performs heap allocations (Array::add,
new ScriptingMessageHolder in event mode). Buffer and UnorderedStack target paths
are allocation-free.
Copies all elements into target container. Accepts Array, Buffer (float-only),
or UnorderedStack (same-mode only). Returns true on success.

Dispatch/mechanics:
  Array: clears target, appends var(float) or new ScriptingMessageHolder per event
  Buffer: float-only, target must be strictly larger than stack size,
    clears buffer then FloatVectorOperations::copy
  UnorderedStack: clearQuick() + insertWithoutSearch() for each element (no dup check)
  Other types: reports "No valid container" script error

Anti-patterns:
  - [BUG] Buffer target requires strictly larger size than stack element count
    (uses < instead of <=). A buffer with exactly the same size fails silently
    and returns false.
  - Do NOT copy event-mode stack to Buffer -- float-only operation

Source:
  ScriptingApiObjects.cpp  copyTo()
    -> Array path: ScriptingApi::Content::Wrapper::create for MessageHolder
    -> Buffer path: data.size() < b->size check, FloatVectorOperations::copy
    -> Stack path: clearQuick() + insertWithoutSearch() (O(1) per element)
