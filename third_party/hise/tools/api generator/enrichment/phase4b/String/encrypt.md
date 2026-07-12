String::encrypt(String key) -> String

Thread safety: UNSAFE -- BlowFish encryption, memory allocation, base64 encoding.
Encrypts the string using BlowFish with the provided key and returns a
base64-encoded result.

Pair with:
  decrypt -- decrypts a string encrypted with the same key

Anti-patterns:
  - Key length is silently clamped to 72 bytes. Keys longer than 72 characters
    are truncated without warning.

Source:
  JavascriptEngineObjects.cpp:771-999  StringClass::encrypt()
    -> MemoryOutputStream -> BlowFish::encrypt(MemoryBlock) -> toBase64Encoding()
