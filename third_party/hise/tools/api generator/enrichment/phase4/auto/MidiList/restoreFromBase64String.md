Loads all 128 slot values from a Base64 string previously produced by `getBase64String()`. The entire array is overwritten in one operation.

> [!Warning:Counter not updated after restore] The internal counter used by `isEmpty()` and `getNumSetValues()` is not updated after restoring. These methods may return stale values until you call `setValue()` on any slot, which forces a counter refresh.
