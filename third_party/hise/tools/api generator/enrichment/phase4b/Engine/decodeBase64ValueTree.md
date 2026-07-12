Engine::decodeBase64ValueTree(String b64Data) -> String

Thread safety: UNSAFE -- Base64 decoding, zstd decompression, ValueTree parsing (heap allocations)
Decodes a Base64-encoded ValueTree (e.g., HISE snippet) and returns XML string. Tries three
decode strategies in sequence. Returns empty string if all fail.
Anti-patterns:
  - Do NOT assume failure means error -- returns empty string for both "empty tree" and
    "all decoding failed" with no way to distinguish
Source:
  ScriptingApi.cpp  Engine::decodeBase64ValueTree()
    -> tries ValueTreeConverters, zstd decompress, raw MemoryBlock
