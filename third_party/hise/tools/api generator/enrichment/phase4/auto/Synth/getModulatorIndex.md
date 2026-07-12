Returns the zero-based index of a modulator within the specified modulation chain of the parent synth. The `chainId` parameter uses 1 for the gain chain and 2 for the pitch chain. Use the returned index with `Synth.setModulatorAttribute()` for positional access to modulators.

> [!Warning:Chain IDs are 1-based not 0-based] The `chainId` does not start at 0. Passing 0 produces an error. Use 1 for GainModulation and 2 for PitchModulation.
