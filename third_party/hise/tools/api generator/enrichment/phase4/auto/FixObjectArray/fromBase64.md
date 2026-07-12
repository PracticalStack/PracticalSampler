Restores the array's data from a Base64-encoded string previously produced by `toBase64()`. Returns 1 on success. If the decoded data size does not match the array's allocation, the array is left unchanged and the method returns 0.

> [!Warning:Size mismatch fails silently] Size mismatch is silent - no script error is thrown. If the factory layout changed between saving and loading (different properties, different element count, different types), the restore quietly fails. Always check the return value.
