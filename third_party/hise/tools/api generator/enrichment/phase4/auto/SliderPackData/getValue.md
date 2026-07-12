Returns the slider value at the given zero-based index.

> [!Warning:Out-of-range returns default silently] Out-of-range indices silently return the default value (1.0) instead of throwing an error. This can mask off-by-one bugs since you get a plausible float value rather than an error message.
