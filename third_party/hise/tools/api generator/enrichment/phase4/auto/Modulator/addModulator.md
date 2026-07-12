Adds a new child modulator of the given type to one of this module's internal chains. Returns a Modulator handle for the newly created modulator. The `chainIndex` depends on the target module type - gain is typically 0, but other indices vary by module.

> [!Warning:Clean up unused modulators explicitly] Created modulators persist in the module tree until explicitly removed with `Synth.removeModulator()`. Forgetting to clean up unused modulators causes accumulating CPU overhead.
