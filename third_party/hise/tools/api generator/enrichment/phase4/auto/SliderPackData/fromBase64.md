Restores slider pack data from a Base64-encoded string previously produced by `toBase64()`. The slider count adjusts to match the decoded data, replacing the current buffer entirely. Use this for restoring custom preset data or implementing clipboard paste.

> [!Warning:Empty string silently ignored] An empty string is silently ignored - the data remains unchanged with no error. Validate input strings before calling if you need to distinguish between "no data" and "restored data".
