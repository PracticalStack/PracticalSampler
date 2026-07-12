Path::loadFromData(NotUndefined data) -> undefined

Thread safety: UNSAFE -- allocates heap memory for base64 decoding, array conversion, or path data parsing
Loads path geometry from external data, replacing the current path. Accepts
three formats: a base64 string (decoded as binary JUCE path data), an array
of byte values (0-255, loaded as raw binary), or another Path object (copies
its geometry directly).

Dispatch/mechanics:
  ApiHelpers::loadPathFromData(p, data):
    String -> MemoryBlock::fromBase64Encoding() -> Path::loadPathFromData()
    Array -> cast each element to unsigned char -> Path::loadPathFromData()
    PathObject -> getPath() direct copy

Pair with:
  Content.createPath(data) -- passes data directly to loadFromData, one-liner shorthand
  toBase64 -- produces base64 string that loadFromData consumes
  fromString -- alternative for human-readable string format

Anti-patterns:
  - If data is not a String, Array, or Path (e.g., a number or boolean), the
    method silently does nothing. No error is reported.

Source:
  ScriptingGraphics.cpp  PathObject::loadFromData()
    -> ApiHelpers::loadPathFromData(p, data)
    -> format detection: isString() / isArray() / dynamic_cast<PathObject*>
