Sets the post-processing effects chain applied to wavetable cycles after resynthesis. The input is an array of JSON objects, each defining one processor. Processors are applied serially in order of definition, and the result is baked into the wavetable with proper band-limiting via mip-mapping.

Each processor receives a normalised cycle position (0.0 to 1.0) as its parameter input. The `min`, `max`, and `middlePosition` properties map this position to the processor's parameter range. An optional Table connection (via `TableProcessor` and `TableIndex`) provides additional curve shaping for the parameter lookup.

Each processor object requires a `Type` property. Available types:

| Type | Description |
| --- | --- |
| `"Sin"` | Sinusoidal waveshaping; adds harmonics without heavy distortion |
| `"Warp"` | Skews the waveform towards the start or end of the cycle |
| `"Fold"` | Wavefolding at the parameter threshold |
| `"Clip"` | Hard clipping (does not normalise afterwards) |
| `"Tanh"` | Soft-clipping saturation |
| `"Bitcrush"` | Amplitude quantisation |
| `"SampleAndHold"` | Sample rate reduction |
| `"Sync"` | Hard sync oscillator effect |
| `"Phase"` | Phase rotation (introduces pitch changes when modulating table index) |
| `"FM1"` / `"FM2"` / `"FM3"` / `"FM4"` | FM with sine carrier at 1x-4x base frequency |
| `"Root"` | Adds a sine wave at the root frequency (negative values subtract) |
| `"Normalise"` | Flattens gain differences between cycles |
| `"Custom"` | Custom waveshaping via a connected Table lookup |

Setting a constant effect uses equal `min` and `max` values. For effects that vary across the wavetable, set different `min` and `max` values and the parameter interpolates linearly across cycles (or through a Table curve if connected).
