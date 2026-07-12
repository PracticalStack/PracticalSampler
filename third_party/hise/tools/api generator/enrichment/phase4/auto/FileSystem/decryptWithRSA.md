Decrypts a hex-encoded string using an RSA public key and returns the original plaintext. The input must be a hexadecimal string as produced by `FileSystem.encryptWithRSA()`. This is a raw RSA operation - the maximum data size is limited by the RSA key size.

> [!Warning:Returns empty string on failure] Returns an empty string silently when the key is invalid or when the decrypted data is not valid UTF-8. There is no error message to indicate failure - always check the return value.
