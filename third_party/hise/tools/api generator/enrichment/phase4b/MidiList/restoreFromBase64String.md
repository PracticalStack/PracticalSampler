MidiList::restoreFromBase64String(String base64encodedValues) -> undefined

Thread safety: WARNING -- String involvement, atomic ref-count operations
Restores all 128 values from a Base64-encoded string created by getBase64String(). Overwrites the raw int[128] memory directly.
Required setup:
  const var list = Engine.createMidiList();
  list.restoreFromBase64String(encoded);
Dispatch/mechanics:
  Creates a MemoryOutputStream pointing to the existing data array (no heap allocation for data), calls Base64::convertFromBase64(stream, base64encodedValues). Does NOT recalculate numValues after restoration.
Pair with: getBase64String -- encode counterpart for round-trip serialization
Anti-patterns:
  - numValues counter is NOT recalculated after restore. isEmpty() and getNumSetValues() may return stale/incorrect values until a subsequent setValue or clear call triggers a counter update.
Source:
  ScriptingApiObjects.cpp:161  restoreFromBase64String() -> MemoryOutputStream(data, 512) -> Base64::convertFromBase64()
