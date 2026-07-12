FileSystem::decryptWithRSA(String dataToDecrypt, String publicKey) -> String

Thread safety: UNSAFE -- BigInteger parsing, RSA key arithmetic, memory block allocation, string construction
Decrypts a hex-encoded string using an RSA public key and returns the original plaintext.
Input must be a hex (base 16) string as produced by encryptWithRSA. Returns empty string
if the key is invalid or decrypted data is not valid UTF-8.

Dispatch/mechanics:
  BigInteger.parseString(dataToDecrypt, 16) -> RSAKey(publicKey)
  -> key.isValid() check -> key.applyToValue(val) -> val.toMemoryBlock()
  -> CharPointer_UTF8::isValidString() check -> mb.toString()

Pair with:
  encryptWithRSA -- encrypt data for later decryption with this method
  getSystemId -- machine ID typically encrypted/decrypted for secure identification

Anti-patterns:
  - Do NOT assume a non-empty return means success -- empty string is returned for both
    invalid keys and non-UTF-8 results, with no error message. Check the return value.
  - Do NOT use for large data -- raw RSA operation, data size limited by key size.

Source:
  ScriptingApi.cpp:7394  FileSystem::decryptWithRSA()
    -> BigInteger.parseString(hex, 16) -> RSAKey.isValid()
    -> RSAKey.applyToValue() -> toMemoryBlock() -> UTF-8 validation
