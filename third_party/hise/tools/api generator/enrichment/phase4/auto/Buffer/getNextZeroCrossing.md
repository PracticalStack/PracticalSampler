Scans forward and returns the next negative-to-positive zero crossing index, or `-1` if none is found. Use it when you need cleaner splice points for edits or loop boundaries.

> [!Warning:Start index not range-clamped] The start index is not range-clamped. Pass a non-negative index inside the buffer.
