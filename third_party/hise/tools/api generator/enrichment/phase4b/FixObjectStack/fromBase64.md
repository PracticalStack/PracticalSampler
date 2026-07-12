FixObjectStack::fromBase64(String b64) -> undefined

Thread safety: UNSAFE -- Base64 decoding involves String parsing and memory block allocation.
Restores the raw memory block from a Base64-encoded string produced by toBase64().
Overwrites ALL allocated slots but does NOT restore the position pointer.

Anti-patterns:
  - [BUG] Does not save or restore the position pointer. After fromBase64(),
    size() returns the pre-restore value. Save size() separately before
    toBase64() and restore the used count externally after fromBase64().

Pair with:
  toBase64 -- produces the Base64 string this method consumes

Source:
  FixLayoutObjects.cpp:1048  Array::fromBase64()
    -> Base64 decodes string into raw memory block (numAllocated bytes)
    -> overwrites all element data, position unchanged
