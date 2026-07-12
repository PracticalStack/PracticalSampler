MidiList::getBase64String() -> String

Thread safety: WARNING -- String involvement, atomic ref-count operations
Serializes all 128 integer values into a Base64-encoded string (encodes raw int[128] memory, 512 bytes). Deterministic output suitable for user presets, XML, or text-based storage.
Required setup:
  const var list = Engine.createMidiList();
  list.fill(42);
  var encoded = list.getBase64String();
Dispatch/mechanics:
  Creates a MemoryOutputStream, calls Base64::convertToBase64(stream, data, sizeof(int)*128), returns stream.toString(). Allocates heap memory for the stream.
Pair with: restoreFromBase64String -- decode counterpart for round-trip serialization
Source:
  ScriptingApiObjects.cpp:154  getBase64String() -> MemoryOutputStream -> Base64::convertToBase64() -> toString()
