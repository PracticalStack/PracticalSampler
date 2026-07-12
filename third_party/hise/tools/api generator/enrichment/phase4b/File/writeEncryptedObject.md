File::writeEncryptedObject(var jsonData, String key) -> Integer

Thread safety: UNSAFE -- performs filesystem I/O (encryption and file write).
Serializes JSON in compact format, encrypts with BlowFish symmetric cipher,
and writes as Base64-encoded text. Returns true on success.

Dispatch/mechanics:
  JSON::toString(jsonData, true) -> MemoryOutputStream
  -> BlowFish(key, jlimit(0,72,key.length())).encrypt(block)
  -> f.replaceWithText(block.toBase64Encoding())

Pair with:
  loadEncryptedObject -- must use the same key for decryption

Anti-patterns:
  - Key length silently clamped to 72 bytes. Keys longer than 72 bytes are
    truncated without warning.
  - Uses BlowFish symmetric encryption, NOT RSA. For public-key encryption,
    use FileSystem.encryptWithRSA / FileSystem.decryptWithRSA.
  - On-disk format is Base64 text (~33% larger than binary).

Source:
  ScriptingApiObjects.cpp  ScriptFile::writeEncryptedObject()
    -> BlowFish(key, jlimit(0,72,size)).encrypt(block)
    -> f.replaceWithText(block.toBase64Encoding())
