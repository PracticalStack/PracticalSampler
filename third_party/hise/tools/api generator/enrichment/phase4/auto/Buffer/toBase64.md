Serialises the buffer into a `Buffer`-prefixed Base64 string for JSON or preset storage. In sparse state models, combine this with a clear empty sentinel so silent lanes are stored as lightweight placeholders.

> [!Warning:Restore limited to 44100 samples] `fromBase64()` rejects payloads larger than 44100 samples, so very large serialised buffers cannot be restored through that path.
