Returns a new Buffer with samples removed from the start and end, leaving the source buffer unchanged. This is commonly used after activity scanning to export only the active tail region.

> [!Warning:Returns new buffer, not in-place] `trim()` does not edit in place. Use the returned buffer in your downstream channel array.
