Returns the internal float data as a Buffer reference for efficient iteration. This is useful for copying values into an array, performing bulk reads for MIDI generation, or analysing pattern data without per-index `getValue()` calls.

> [!Warning:Live reference bypasses change notifications] The returned Buffer is a live reference, not a copy. Writing to it (e.g. `buf[0] = 0.5`) modifies the slider data directly without triggering change notifications. Use `setValue()` or `setAllValues()` if listeners need to be notified.
