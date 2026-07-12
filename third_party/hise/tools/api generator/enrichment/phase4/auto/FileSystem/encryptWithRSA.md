Encrypts a plaintext string using an RSA private key and returns the result as a hexadecimal string. This is a raw RSA operation without hybrid encryption, so keep the plaintext short - machine IDs, expiry dates, and similar sensitive configuration data. Decrypt the result with `FileSystem.decryptWithRSA()` using the corresponding public key.

> [!Warning:Malformed key produces silent garbage] Unlike `FileSystem.decryptWithRSA()`, this method does not validate the key before use. A malformed key produces meaningless output without any error indication.
