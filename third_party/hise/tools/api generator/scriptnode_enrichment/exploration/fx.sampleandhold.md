# fx.sampleandhold -- C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/FXNodes.h:44`
**Base class:** `polyphonic_base`
**Classification:** audio_processor

## Signal Path

Sample rate reduction effect. Holds the current sample value for N samples, where N is the Counter parameter.

The `processFrame()` method implements the core logic per sample:

```
if counter == 0:
    capture current input sample into currentValues (per channel)
    reset counter to factor
else:
    replace input with stored currentValues (per channel)
    decrement counter
```

The `process()` method has a block-level optimisation: if `counter > numSamples`, the entire block can be filled with the held value using `hmath::vmovs()` (vectorised fill), and the counter is decremented by numSamples. Otherwise it falls back to per-sample frame processing via `FrameConverters::forwardToFrame16()`.

At Counter=1: factor=1, so counter resets to 1 every sample. Every sample is captured and immediately output -- pass-through.
At Counter=64: only every 64th sample is captured; the remaining 63 samples repeat the held value. Effective sample rate is reduced by 64x.

## Gap Answers

### counter-mechanism: How does Counter work exactly?

The counter is per-voice (inside `PolyData<Data, NumVoices>`). The `Data` struct holds: `int factor` (the Counter parameter value), `int counter` (current countdown), and `span<float, NUM_MAX_CHANNELS> currentValues` (held sample per channel). The counter is per-voice but the held values are per-channel within each voice. When counter reaches 0, new values are captured from all channels simultaneously, and counter resets to factor. Between captures, all channels output their respective held values.

### counter-at-one: Is Counter=1 pass-through?

Yes. `setCounter()` clamps to `jlimit(1, 44100, roundToInt(value))`. At factor=1, counter resets to 1 after every capture, meaning every sample is captured. This is effectively pass-through.

### processing-model: Per-sample or per-block?

Hybrid. `process()` checks if `counter > numSamples` -- if so, the entire block is filled with held values using vectorised `vmovs()` (fast block fill). Otherwise, falls back to per-sample via `forwardToFrame16()`. This means large Counter values benefit from block-level optimisation.

### existing-doc-error: Confirm Counter is the only parameter

Confirmed. The `Parameters` enum contains only `Counter`. `createParameters()` registers only one parameter with range {1.0, 64.0, 1.0} and default 1.0. The existing phase3 doc listing "Position" is a copy-paste error from haas.md.

## Parameters

- **Counter (enum index 0):** Range 1-64, step 1, default 1. Setter `setCounter()` uses `jlimit(1, 44100, roundToInt(value))` -- the setter accepts up to 44100 even though the parameter range caps at 64. Stored as `int factor` inside the per-voice `Data` struct.

## Polyphonic Behaviour

Full per-voice state via `PolyData<Data, NumVoices>`. Each voice has its own counter position, factor, and held sample values. Voices do not share sample-hold state. The `lastChannelAmount` member (used in reset) is shared across voices.

## CPU Assessment

baseline: negligible
polyphonic: true
scalingFactors: [{"parameter": "Counter", "impact": "lower is slightly more expensive", "note": "Low Counter values force per-sample frame processing; high values use fast block fill"}]

## Notes

The setter `setCounter()` upper-clamps at 44100, not 64 as the parameter range suggests. If Counter is driven by modulation beyond the parameter range, values up to 44100 are accepted. The `Data::clear()` method has a `numChannelsToClear` parameter but does not actually use it -- it always clears all NUM_MAX_CHANNELS entries.
