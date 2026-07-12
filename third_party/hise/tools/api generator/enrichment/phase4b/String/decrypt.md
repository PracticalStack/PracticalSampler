String::decrypt(String key) -> String

Thread safety: UNSAFE -- BlowFish decryption, base64 decoding, string construction.
Decrypts the string using BlowFish with the provided key. Expects base64-encoded
encrypted format as produced by encrypt().

Pair with:
  encrypt -- encrypts a string with BlowFish, producing the format decrypt expects

Anti-patterns:
  - Key length is silently clamped to 72 bytes. Keys longer than 72 characters
    are truncated without warning.

Source:
  JavascriptEngineObjects.cpp:771-999  StringClass::decrypt()
    -> base64 decode -> BlowFish::decrypt(MemoryBlock) -> toString()
