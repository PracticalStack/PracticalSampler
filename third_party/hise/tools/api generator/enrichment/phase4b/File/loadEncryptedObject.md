File::loadEncryptedObject(String key) -> JSON

Thread safety: UNSAFE -- reads file from disk, decrypts with BlowFish, parses JSON (I/O).
Reads a Base64-encoded file, decrypts using BlowFish symmetric encryption with
the provided key, and parses the result as JSON. Silently returns undefined on
failure (wrong key, corrupted file, invalid JSON).

Dispatch/mechanics:
  f.loadFileAsString() -> MemoryBlock::fromBase64Encoding()
  -> BlowFish(key, jlimit(0,72,key.length())).decrypt(block)
  -> JSON::parse(decrypted) -- no error reported on parse failure

Pair with:
  writeEncryptedObject -- must use the same key for round-trip

Anti-patterns:
  - Silently returns undefined on any failure (unlike loadAsObject which reports
    errors). Always check isDefined() on the result.
  - Uses BlowFish symmetric encryption, NOT RSA. For public-key encryption, use
    FileSystem.encryptWithRSA / FileSystem.decryptWithRSA.
  - Key length silently clamped to 72 bytes. Two keys sharing the same first 72
    bytes produce identical encryption.

Source:
  ScriptingApiObjects.cpp  ScriptFile::loadEncryptedObject()
    -> BlowFish(key, jlimit(0,72,size)).decrypt(block)
    -> JSON::parse(block.toString(), v)
