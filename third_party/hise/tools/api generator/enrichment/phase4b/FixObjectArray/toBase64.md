FixObjectArray::toBase64() -> String

Thread safety: WARNING -- string allocation, atomic ref-count operations
Serializes the array's entire raw memory block into a Base64-encoded string.
Binary snapshot of all element data in memory layout order.

Dispatch/mechanics:
  MemoryBlock(data, numAllocated) -> toBase64Encoding()
  Encodes raw bytes, not JSON-style property values

Pair with:
  fromBase64 -- restores the array from the encoded string

Anti-patterns:
  - Do NOT assume the encoded string is portable across factory layout changes --
    different prototypes produce different allocation sizes, and fromBase64 will
    silently reject the string

Source:
  FixLayoutObjects.cpp:1048  Array::toBase64()
    -> MemoryBlock(data, numAllocated) -> toBase64Encoding()
