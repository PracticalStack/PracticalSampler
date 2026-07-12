FixObjectArray::fromBase64(String b64) -> Integer

Thread safety: SAFE
Restores the array's raw memory from a Base64-encoded string. Validates that
decoded size matches the array's allocation (elementSize * numElements) exactly.
Returns 1 on success, 0 on size mismatch (silent failure).

Dispatch/mechanics:
  Base64 decode -> MemoryBlock -> size check against numAllocated
  Match: memcpy into data pointer, returns 1
  Mismatch: array unchanged, returns 0 (no error thrown)

Pair with:
  toBase64 -- produces the string that fromBase64 consumes

Anti-patterns:
  - Do NOT assume fromBase64 throws on failure -- size mismatch is silent, always check
    the return value
  - Do NOT change the factory prototype between save and load -- different layout means
    different allocation size, causing silent rejection

Source:
  FixLayoutObjects.cpp:1054  Array::fromBase64()
    -> MemoryBlock::fromBase64Encoding()
    -> size validation against numAllocated
    -> memcpy on match
