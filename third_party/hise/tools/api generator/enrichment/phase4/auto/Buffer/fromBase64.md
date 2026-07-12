Decodes a string created by `toBase64()`, resizes this buffer to the decoded length, and copies in the sample data. It returns `1` for success and `0` for invalid payloads, so you can use it directly as a restore gate in import loops.

> [!Warning:Input requires Buffer prefix string] Input must include the `Buffer` prefix. Plain Base64 text returns `0` and leaves your restore step incomplete if you do not check the return value.
