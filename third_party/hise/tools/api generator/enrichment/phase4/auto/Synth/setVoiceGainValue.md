Applies a gain factor to a specific voice identified by its zero-based voice index. The value is a linear gain multiplier (1.0 = unity) that is applied alongside the standard modulation chain output. This method is intended for use inside Script Voice Start Modulators where the `voiceIndex` variable is available.

> [!Warning:Out-of-range indices silently clamped] Out-of-range voice indices are silently ignored - no error is produced. Negative values are clamped to 0, which quietly sets the gain on voice 0 instead of reporting an error.
