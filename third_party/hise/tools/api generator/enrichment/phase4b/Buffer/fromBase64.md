Buffer::fromBase64(String b64String) -> Integer

Thread safety: UNSAFE
Decodes a Buffer-prefixed Base64 payload, resizes this buffer, and copies decoded float data.
Returns 1 on success, 0 on invalid prefix or decode failure.
Dispatch/mechanics: Validates literal Buffer prefix, decodes payload into MemoryBlock, resizes internal storage, then copies raw floats into this VariantBuffer.
Pair with: toBase64 (round-trip serialization pair)
Anti-patterns:
  - Do NOT pass plain Base64 text without the Buffer prefix -- method returns 0 silently.
  - Do NOT decode placeholder entries in sparse storage workflows -- check sentinel first and skip decode.
Source:
  VariantBuffer.cpp:172  VariantBuffer::addMethods() -> setMethod("fromBase64", lambda)
