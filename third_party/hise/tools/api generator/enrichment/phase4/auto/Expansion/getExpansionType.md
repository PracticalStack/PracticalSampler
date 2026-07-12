Returns the expansion type as an integer. Compare the result against the constants on `ExpansionHandler`:

| Constant | Value | Description |
|----------|-------|-------------|
| `ExpansionHandler.FileBased` | 0 | Unencoded folder-based expansion |
| `ExpansionHandler.Intermediate` | 1 | Encoded `.hxi` expansion |
| `ExpansionHandler.Encrypted` | 2 | Encrypted `.hxp` expansion |

> [!Warning:Returns -1 for invalid references] If the expansion has been unloaded or deleted, this returns -1 rather than a valid type constant. Always compare against the `ExpansionHandler` constants rather than assuming the return value is in range.
