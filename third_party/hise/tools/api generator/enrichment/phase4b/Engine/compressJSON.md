Engine::compressJSON(var object) -> String

Thread safety: UNSAFE -- JSON serialization, zstd compression, Base64 encoding (heap allocations)
Converts a JSON object to a zstd-compressed Base64 string. Use uncompressJSON() to reverse.
Pair with:
  uncompressJSON -- decode the compressed string back to JSON
Source:
  ScriptingApi.cpp  Engine::compressJSON()
    -> JSON::toString() -> zstd compress -> Base64 encode
