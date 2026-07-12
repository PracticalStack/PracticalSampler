# core.mono2stereo - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/CoreNodes.h:507`
**Base class:** `HiseDspBase`
**Classification:** utility

## Signal Path

Copies channel 0 to channel 1. In `process()` (line 521): if 2+ channels exist, copies channel 0 data to channel 1 using `Math.vmov(dst, data[0])`. In `processFrame()` (line 531): `data[1] = data[0]`.

Channel 0 is not modified. Channels beyond 1 are not modified. If there is only 1 channel, nothing happens.

## Gap Answers

### copy-mechanism: What exactly does it do?

Copies channel 0 to channel 1, overwriting whatever was in channel 1. The `Math.vmov()` call is a JUCE FloatVectorOperations memory copy. If the input already has 2 channels with different content, channel 1 is overwritten with channel 0.

### channel-count-change: Does it change channel count?

No. It assumes the buffer already has 2+ channels and copies data between them. It does not add channels. In scriptnode, channel count is determined by the container context, so the node must be placed where 2 channels are available.

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []
