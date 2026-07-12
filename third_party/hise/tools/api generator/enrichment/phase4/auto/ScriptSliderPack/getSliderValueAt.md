Returns the value at one slider index and updates the displayed index highlight used by the UI.

This is the usual way to fetch lane values inside index-driven control callbacks.

> [!Warning:Out-of-range returns default silently] Out-of-range indices do not throw an error - they return the slider-pack default value.
