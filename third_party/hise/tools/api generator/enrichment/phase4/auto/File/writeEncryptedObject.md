Serialises a JSON object, encrypts it with BlowFish symmetric encryption, and writes the result as a Base64-encoded string to this file. The same key must be used for both encryption and decryption via `loadEncryptedObject`. This is useful for storing data that should not be trivially readable from disk, such as offline licence activation data that bypasses an online check.

> [!Warning:Key silently truncated to 72 bytes] The key length is silently clamped to 72 bytes. Longer keys are truncated without warning, so two keys that share the same first 72 bytes produce identical encryption.
