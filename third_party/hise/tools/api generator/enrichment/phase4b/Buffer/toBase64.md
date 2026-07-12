Buffer::toBase64() -> String

Thread safety: UNSAFE
Serializes raw float sample data and returns a Base64 string prefixed with Buffer.
Use for compact persistence of fixed-size buffer state.
Dispatch/mechanics: Packs raw float bytes into MemoryBlock, Base64-encodes payload, then prepends literal Buffer prefix used by fromBase64.
Pair with: fromBase64 (restore serialized state)
Anti-patterns:
  - Do NOT assume very large payloads always round-trip -- fromBase64 currently rejects decoded payloads above 44100 samples.
Source:
  VariantBuffer.cpp:172  VariantBuffer::addMethods() -> setMethod("toBase64", lambda)
