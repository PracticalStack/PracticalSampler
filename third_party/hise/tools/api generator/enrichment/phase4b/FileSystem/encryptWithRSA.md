FileSystem::encryptWithRSA(String dataToEncrypt, String privateKey) -> String

Thread safety: UNSAFE -- BigInteger arithmetic, memory block allocation, string construction
Encrypts a plaintext string using an RSA private key and returns the result as a hex (base 16)
string. Raw RSA operation without hybrid encryption -- data size limited by RSA key size.
Keep plaintext short (machine IDs, expiry dates, configuration tokens).

Dispatch/mechanics:
  MemoryOutputStream << dataToEncrypt -> BigInteger.loadFromMemoryBlock()
  -> RSAKey(privateKey).applyToValue(val) -> val.toString(16)

Pair with:
  decryptWithRSA -- decrypt data encrypted with this method
  getSystemId -- machine ID typically used as part of encrypted data payloads

Anti-patterns:
  - Do NOT assume key validation occurs -- unlike decryptWithRSA, this method does not call
    RSAKey::isValid(). A malformed key produces garbage output without error.
  - Do NOT use for large data -- raw RSA operation, data size limited by key size.

Source:
  ScriptingApi.cpp:7394  FileSystem::encryptWithRSA()
    -> MemoryOutputStream -> BigInteger.loadFromMemoryBlock()
    -> RSAKey(privateKey).applyToValue() -> toString(16)
