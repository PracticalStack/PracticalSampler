# fx.reverb -- C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/FXNodes.h:299` (class), `FXNodes.cpp:40` (implementation)
**Base class:** `HiseDspBase`
**Classification:** audio_processor

## Signal Path

Thin wrapper around `juce::Reverb`. The JUCE Reverb is a Freeverb-style algorithm (8 parallel comb filters + 4 series allpass filters).

`process()` dispatches based on channel count:
- 1 channel: `r.processMono(data, numSamples)`
- 2+ channels: `r.processStereo(left, right, numSamples)`

`processFrame()` does the same but with 1 sample at a time.

The node operates as 100% wet. In the constructor, `dryLevel` is explicitly set to 0.0:

```cpp
auto p = r.getParameters();
p.dryLevel = 0.0f;
r.setParameters(p);
```

The `wetLevel` is left at the JUCE default (0.33). `freezeMode` is left at default (0.0, disabled). These are not exposed as parameters.

## Gap Answers

### juce-reverb-wrapper: How does the node wrap juce::Reverb?

Direct wrapping. The three exposed parameters map to `juce::Reverb::Parameters` fields:
- Size -> `roomSize` (direct mapping, no transformation)
- Damping -> `damping` (direct mapping)
- Width -> `damping` (**BUG** -- see Issues below)

All setters use `jlimit(0.0f, 1.0f, value)`. `prepare()` calls `r.setSampleRate()`. `reset()` calls `r.reset()`.

### missing-parameters: What about wetLevel, dryLevel, freezeMode?

- `dryLevel` is hardcoded to 0.0 in the constructor (100% wet output)
- `wetLevel` is left at JUCE default (0.33)
- `freezeMode` is left at JUCE default (0.0, disabled)
- None of these are exposed as parameters

Users who need dry/wet control should use a `container.split` or the `dry_wet` template wrapper. The wetLevel default of 0.33 means the reverb output is attenuated -- this is the JUCE Reverb's internal gain staging, not a mix control.

### not-polyphonic: Why no polyphonic support?

The class inherits from `HiseDspBase` only (not `polyphonic_base`). It uses `SN_NODE_ID` (not `SN_POLY_NODE_ID`) and `isPolyphonic()` explicitly returns false. The `juce::Reverb` instance is a single shared object -- no per-voice state. This is intentional: reverb is typically a shared bus effect, not a per-voice processor. The internal state (comb filter delays, allpass buffers) would be expensive to duplicate per voice and musically inappropriate.

### channel-handling: Mono or stereo?

Both. `process()` checks `d.getNumChannels()`: if 1, uses `processMono()`; otherwise uses `processStereo()` with channels 0 and 1. Extra channels beyond 2 are ignored. The JUCE Reverb's stereo processing adds Width-based decorrelation between channels.

## Parameters

- **Damping (enum index 0):** Range 0-1, default 0.5, NormalizedPercentage. Maps directly to `juce::Reverb::Parameters::damping`. Controls high-frequency absorption in the reverb tail.
- **Width (enum index 1):** Range 0-1, default 0.5, NormalizedPercentage. **BUG:** `setWidth()` sets `p.damping` instead of `p.width`. The Width parameter actually controls damping a second time, and the actual stereo width is stuck at the JUCE default.
- **Size (enum index 2):** Range 0-1, default 0.5, NormalizedPercentage. Maps to `juce::Reverb::Parameters::roomSize`. Controls the perceived size of the reverberant space.

## CPU Assessment

baseline: medium
polyphonic: false
scalingFactors: []

JUCE Reverb processes 8 comb filters + 4 allpass filters per sample. Moderate CPU cost. Fixed cost regardless of parameter values. Not duplicated per voice.

## Notes

The parameter enum order is Damping(0), Width(1), Size(2) -- but the preliminary JSON lists them as Size, Damping, Width. The C++ enum order determines the actual parameter indices. The `createParameters()` adds them in order: Damping, Width, Size.
