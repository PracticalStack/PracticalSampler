Sets an attribute on a modulator within the parent synth's gain or pitch chain, identified by chain type and positional index. Use `chainId` 1 for the gain chain and 2 for the pitch chain. The `modulatorIndex` is the zero-based position of the modulator in the chain (as returned by `getModulatorIndex`).

Two special `attributeIndex` values provide shortcuts to common properties:

| attributeIndex | Property | Notes |
|----------------|----------|-------|
| -12 | Intensity | For the pitch chain, the value is in semitones (-12 to +12) and is converted to a ratio internally. For the gain chain, the value is passed directly. |
| -13 | Bypassed | Pass 1.0 to bypass, 0.0 to enable. |

Positive values address standard processor attributes.

> [!Warning:Chain IDs are 1-based not 0-based] The `chainId` does not start at 0. Passing 0 produces an error. Use 1 for GainModulation and 2 for PitchModulation.
