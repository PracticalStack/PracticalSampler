Path::toBase64() -> String

Thread safety: UNSAFE -- allocates heap memory for MemoryOutputStream, binary serialization, and base64 encoding
Serializes the path to a compact base64 string. More compact than toString().
The output can be stored, transmitted, or used as a CSS/StyleSheet path
property value. Restore via loadFromData(base64String). Returns empty string
for an empty path.

Pair with:
  loadFromData -- restores a path from the base64 string
  toString -- alternative human-readable format (less compact)

Source:
  ScriptingGraphics.cpp  PathObject::toBase64()
    -> MemoryOutputStream mos
    -> p.writePathToStream(mos)
    -> mos.getMemoryBlock().toBase64Encoding()
