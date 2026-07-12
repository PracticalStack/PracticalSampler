Maps a display row index back to the original data array index. When the table is sorted, display row indices no longer match the original data order. Use this after every callback that mutates the underlying data to get the correct index.

> [!Warning:O(n) search, cache for large tables] This method performs an O(n) search internally. For large tables (thousands of rows), avoid calling it in a tight loop. Cache the mapping if called frequently.
