Dynamically adds a modulator to the parent synth's gain or pitch modulation chain and returns a `ScriptModulator` handle. If a modulator with the same `id` already exists in the target chain, the existing processor is returned. The `chainId` parameter uses 1 for the gain chain and 2 for the pitch chain. Audio processing is briefly suspended during the operation.

> [!Warning:Chain IDs are 1-based not 0-based] The `chainId` does not start at 0. Passing 0 produces an error. Use 1 for GainModulation and 2 for PitchModulation.
