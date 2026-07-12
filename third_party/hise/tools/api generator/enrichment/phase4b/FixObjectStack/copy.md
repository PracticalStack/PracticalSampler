FixObjectStack::copy(String propertyName, AudioData target) -> undefined

Thread safety: WARNING -- String involvement, atomic ref-count operations. Iterates full capacity O(length).
Copies the named property from each element into a target Buffer or Array. Reads
from ALL allocated slots (full capacity), not just the used portion up to size().

Anti-patterns:
  - Do NOT assume copy() respects size() -- it reads all allocated slots including
    unused ones beyond position. Unused slots contain default values (0 for
    int/float, false for bool). Use a manual loop from 0 to size() for used-only.

Pair with:
  size -- to manually iterate only the used portion
  fill -- to pre-fill all slots with known values before copy()

Source:
  FixLayoutObjects.cpp:995  Array::copy()
    -> iterates numElements (full capacity), not size()
    -> reads named property from each ObjectReference into target buffer
