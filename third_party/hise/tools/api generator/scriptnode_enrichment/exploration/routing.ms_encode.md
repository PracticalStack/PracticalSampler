# routing.ms_encode - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/RoutingNodes.h:843`
**Base class:** `HiseDspBase`
**Classification:** audio_processor

## Signal Path

Converts a stereo Left/Right signal to Mid/Side representation using frame-based processing. The `process()` method forwards to `FrameConverters::forwardToFrameStereo()` which dispatches to `processFrame()` per sample.

In `processFrame()` (line 862-874), when the frame has exactly 2 channels:
```
M = (L + R) * 0.5
S = (L - R) * 0.5
```
Channel 0 (left) is replaced with Mid, channel 1 (right) is replaced with Side.

If the frame does not have exactly 2 channels, no processing occurs (the `if (data.size() == 2)` guard at line 863 skips non-stereo signals).

## Gap Answers

### encoding-formula: What exact formula is used for the MS encoding?

The formula uses 0.5 scaling:
- `M = (L + R) * 0.5f` (line 868)
- `S = (L - R) * 0.5f` (line 869)

This pairs with ms_decode which uses NO 0.5 scaling (`L = M + S`, `R = M - S`). The round-trip is gain-neutral: encode then decode returns the original signal. Proof: `L_out = M + S = (L+R)*0.5 + (L-R)*0.5 = L`. `R_out = M - S = (L+R)*0.5 - (L-R)*0.5 = R`.

### channel-count-requirement: Does this node require exactly 2 channels? What happens with mono or multichannel signals?

The node only processes when `data.size() == 2`. With mono (1 channel), 3 channels, or 4 channels, the signal passes through completely unmodified. `forwardToFrameStereo` dispatches 1, 2, or 4 channels, but the `processFrame` body only acts on exactly 2. No error is raised for non-stereo signals.

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []
