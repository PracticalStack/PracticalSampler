FixObjectStack::toBase64() -> String

Thread safety: UNSAFE -- Base64 encoding creates a new String with heap allocation.
Serializes the entire raw memory block (all allocated slots) as a Base64-encoded
string. Includes data from unused slots beyond the position pointer. Does NOT
encode the position value.

Anti-patterns:
  - [BUG] Serializes the full memory block including unused slots. The position
    pointer is not included in the output. Save size() separately and restore the
    used count externally alongside the Base64 string.

Pair with:
  fromBase64 -- restores the memory block from the string this method produces

Source:
  FixLayoutObjects.cpp:1048  Array::toBase64()
    -> encodes raw memory block (numAllocated bytes) as Base64
    -> position not included in output
