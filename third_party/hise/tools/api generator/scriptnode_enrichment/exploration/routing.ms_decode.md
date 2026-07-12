# routing.ms_decode - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/RoutingNodes.h:877`
**Base class:** `HiseDspBase`
**Classification:** audio_processor

## Signal Path

Converts a Mid/Side signal back to stereo Left/Right using frame-based processing. The `process()` method forwards to `FrameConverters::forwardToFrameStereo()` which dispatches to `processFrame()` per sample.

In `processFrame()` (line 895-908), when the frame has exactly 2 channels:
```
L = M + S
R = M - S
```
Channel 0 (mid) is replaced with Left, channel 1 (side) is replaced with Right.

## Gap Answers

### decoding-formula: What exact formula is used for the MS decoding? Does it match ms_encode for gain neutrality?

The formula uses NO scaling factor:
- `L = M + S` (line 903: `auto l = (m + s)`)
- `R = M - S` (line 904: `auto r = (m - s)`)

This is the correct algebraic inverse of ms_encode's formula (which uses 0.5 scaling). The encode/decode pair is gain-neutral. Round-trip: encode(`L,R`) = `((L+R)*0.5, (L-R)*0.5)`, then decode gives `(L+R)*0.5 + (L-R)*0.5 = L` and `(L+R)*0.5 - (L-R)*0.5 = R`.

### channel-count-requirement: Does this node require exactly 2 channels? What happens with non-stereo signals?

Same as ms_encode: processing only occurs when `data.size() == 2` (line 896). Non-stereo signals pass through unmodified. No error is raised.

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []
