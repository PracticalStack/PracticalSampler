Encrypts the string using BlowFish with the provided key and returns a base64-encoded result. Use `String.decrypt()` with the same key to recover the original. This provides obfuscation rather than cryptographic security - suitable for licence strings or saved configuration data.

> [!Warning:Key length silently clamped to 72 bytes] Keys longer than 72 characters are truncated without warning. Both `encrypt` and `decrypt` use only the first 72 bytes, so the roundtrip still works, but the extra key material has no effect.
