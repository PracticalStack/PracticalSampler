Engine::uncompressJSON(String b64) -> JSON

Thread safety: UNSAFE -- Base64 decoding, zstd decompression, JSON parsing
Decompresses Base64+zstd string back to JSON object. Inverse of compressJSON().
Anti-patterns:
  - Passing arbitrary Base64 not from compressJSON produces decompression or parse errors
Pair with:
  compressJSON -- create the compressed string
Source:
  ScriptingApi.cpp  Engine::uncompressJSON()
    -> Base64 decode -> zstd decompress -> JSON::parse()
