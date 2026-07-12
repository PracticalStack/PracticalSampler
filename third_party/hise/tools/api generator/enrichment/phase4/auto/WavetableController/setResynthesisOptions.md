Sets the resynthesis options from a JSON object. Properties not specified in the object retain their defaults. The options control how audio data is analysed and converted into wavetable cycles during the next `resynthesise()` call. Setting options does not trigger resynthesis automatically - call `resynthesise()` afterwards.

The `PhaseMode` property controls phase alignment and has four modes:

| Mode | Description |
| --- | --- |
| `"Resample"` | Simple resampling without phase alignment |
| `"ZeroPhase"` | Ignores phase; treats every harmonic as a zero-phase sine |
| `"StaticPhase"` | Preserves the first cycle's phase across all cycles (recommended default) |
| `"DynamicPhase"` | Tracks phase per cycle; best for organic material but may introduce pitch wobble |

Other key properties: `NumCycles` (fixed count or -1 for auto, clamped to max 512 and rounded to next power of two), `RemoveNoise` (SiTraNo noise separation), `UseLoris` (Loris-based resynthesis, requires `HISE_INCLUDE_LORIS` - note the GPL licence), `RootNote` (pitch detection override, -1 for auto), `ReverseOrder` (reverses cycle ordering), and `MipMapSize` (semitones per mipmap band, default 12).

> [!Warning:Reset ForceResynthesis after use] When using `ForceResynthesis = true` for cross-oscillator resampling or debugging, reset it to `false` afterwards. Leaving it enabled bypasses the resynthesis cache and causes unnecessary re-processing on every wavetable load.
