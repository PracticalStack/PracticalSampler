FixObjectArray::copy(String propertyName, ScriptObject target) -> Integer

Thread safety: UNSAFE -- heap operations (Array::ensureStorageAllocated, Array::set)
Extracts a single named property from every element into a target Buffer or Array.
Returns 1 on success, 0 on failure.

Required setup:
  const var buf = Buffer.create(a.length);  // Buffer size must match array length

Dispatch/mechanics:
  Looks up property by name in layout -> gets offset and type
  Buffer target: casts each value to float, writes sequentially
  Array target: resizes via ensureStorageAllocated, populates via set()
  Reads directly from raw memory using property offset, stepping by elementSize

Pair with:
  fill -- the inverse direction (broadcast a value into all slots)

Anti-patterns:
  - Do NOT use a Buffer with a different size than the array's length -- produces a script error
  - Do NOT expect integer precision beyond float32 range when copying to Buffer --
    values beyond +/-16777216 lose precision due to float cast
  - Do NOT pass a target that is neither Buffer nor Array -- silently returns 0

Source:
  FixLayoutObjects.cpp:995  Array::copy()
    -> layout lookup for property offset/type
    -> Buffer path: size check, then per-element float cast + write
    -> Array path: ensureStorageAllocated + set per element
