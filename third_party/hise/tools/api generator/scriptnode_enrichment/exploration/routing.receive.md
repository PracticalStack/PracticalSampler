# routing.receive - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/RoutingNodes.h:633`
**Base class:** `receive_base` (inherits `routing::base` -> `mothernode` + `polyphonic_base`)
**Classification:** audio_processor

## Signal Path

The receive node reads audio from a connected send node's cable buffer and adds it to the current signal, scaled by the Feedback parameter. The receive does NOT replace the existing signal -- it adds to it.

For block cables: `process()` calls `getTypedSource().readIntoBuffer(data, feedback.get())` (line 721). In `readIntoBuffer()`, `FloatVectorOperations::addWithMultiply(dst, src, feedback, numThisTime)` is used (line 201), which performs `dst += src * feedback` -- an additive mix scaled by feedback.

For frame cables: `processFrame()` adds each sample from the source frame data scaled by feedback: `d += sourceData[index++] * fb` (line 738).

## Gap Answers

### feedback-mixing-behaviour: Does the Feedback parameter control a wet/dry mix between the node's input signal and the received signal, or does it scale only the received signal which is then added to the input?

Feedback scales only the received signal, which is then added to whatever is already in the audio buffer. The formula is: `output = input + received_signal * Feedback`. This is NOT a wet/dry crossfade. At Feedback=0, the received signal contributes nothing. At Feedback=1, the full received signal is added. The existing signal in the buffer is always preserved.

### receive-without-send: What happens when a receive node has no connected send node? Does it output silence, pass through its input unchanged, or produce an error?

When no send is connected, the `source` pointer points to the `null` cable member (line 771: `cable::cable_base* source = &null`). The null cable is a default-constructed CableType. For block cables, `readIntoBuffer()` checks `if(d.buffer.isEmpty()) return` (line 181-182), so nothing is added. For frame cables, the null cable's `frameData` is zero-initialised via `reset()` which does `memset(fd.begin(), 0, sizeof(fd))` (line 142). So the receive passes through its input signal unchanged when disconnected.

### feedback-loop-latency: When used in a feedback loop (send after processing, receive before), is there inherent latency (one block delay) or is the feedback instantaneous within the same block?

There is inherent latency. The cable uses separate read and write indices in a circular buffer (`readIndex` and `writeIndex` in `block_base::Data`, lines 277-278). The send writes to `writeIndex` and the receive reads from `readIndex`. Both start at 0 and advance independently. Since the send writes the current block's data and the receive reads from the same position, in a feedback configuration (receive before send in the chain), the receive reads data from the previous block because the send has not yet written the current block. This results in one block of latency. For frame-mode cables, the same-frame feedback could theoretically work since frame data is written per-sample, but the typical usage pattern still implies one-block delay.

### polyphonic-receive-routing: With IsPolyphonic true, does each voice receive only from the same voice's send, or from all voices?

Each voice receives only from the same voice's send. The cable's internal storage uses `PolyData`, and both send and receive call `.get()` which returns the current voice's data. Voice isolation is maintained -- voice N's receive reads from voice N's cable buffer.

## Parameters

- **Feedback** (0.0 -- 1.0, default 0.0, NormalizedPercentage): Scales the received signal before adding it to the current buffer. At 0 the received signal is muted; at 1 the full signal is added.

## Polyphonic Behaviour

`PolyData<float, NumVoices> feedback` stores per-voice feedback values. `setFeedback()` iterates all voices or the current voice depending on context. The cable's internal buffer storage is also per-voice via PolyData.

## CPU Assessment

baseline: negligible
polyphonic: true
scalingFactors: []

## Notes

The `reset()` method resets the source cable (line 670-672: `source->reset()`), which clears the cable's internal buffer and resets read/write indices. The Feedback parameter default is 0.0 in the code (no explicit `setDefaultValue` call, so uses range default), meaning a newly created receive node will not pass any received signal until Feedback is increased.
