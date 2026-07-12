# routing.global_send - C++ Exploration

**Source:** `hi_scripting/scripting/scriptnode/dynamic_elements/GlobalRoutingNodes.h:85` (declaration), `GlobalRoutingManager.cpp:1528` (implementation)
**Base class:** `GlobalRoutingNodeBase`
**Classification:** audio_processor

## Signal Path

Copies audio from the local signal chain into a shared GlobalRoutingManager Signal buffer. The local signal continues unmodified after the send -- the node does NOT consume or silence the signal. The copy is gain-scaled by the Value parameter.

`process()`: acquires `ScopedTryReadLock(connectionLock)`, checks `currentSlot != nullptr && !isBypassed()`, then calls `currentSlot->push(data, value)`.

`Signal::push()` uses `FloatVectorOperations::copyWithMultiply(channelData[i], data[i], value, numSamples)` -- this overwrites the shared buffer with the gain-scaled input.

## Gap Answers

### signal-copy-vs-consume: Does global_send copy the audio to the shared buffer (signal continues in the local chain) or consume it (silence after the send)?

Copy. The `push()` method reads from the ProcessDataDyn channels and writes into the Signal's shared buffer using `copyWithMultiply`. It does not modify the input ProcessDataDyn at all. The local signal path continues with the original signal unmodified after the send.

### value-as-gain: The Value parameter (0-1) is used as the gain multiplier in push(). Is this linear gain or does it use any curve/conversion?

Linear gain. `push()` calls `FloatVectorOperations::copyWithMultiply(dst, src, value, numSamples)` directly. No dB conversion or curve is applied. The `value` float member is set directly from the parameter callback `setValue(void* obj, double v)` which just does `typed->value = v`.

### uncompileable-reason: The node is marked UncompileableNode. What is the architectural reason it cannot be compiled to C++?

The constructor explicitly sets `CustomNodeProperties::setPropertyForObject(*this, PropertyIds::UncompileableNode)`. The architectural reason is that `GlobalSendNode` inherits from `GlobalRoutingNodeBase` which in turn inherits from `NodeBase` -- it relies on the full scriptnode IDE infrastructure including `DspNetwork`, `GlobalRoutingManager` singleton access, and ValueTree-based slot management. These are not available in the compiled (hi_dsp_library) context. For control values, the compiled `routing::global_cable` provides an alternative via the runtime_target system, but there is no compiled equivalent for full audio buffer routing.

### one-sender-per-slot: From infrastructure: only one send node per signal slot. What error/behaviour occurs if a second global_send tries to connect to an occupied slot?

`Signal::setSource()` checks if a different send node is already connected. If `sendNode != nullptr` and `sendNode != src`, it returns a Result with error message "Slot already has a send node". The new connection is rejected and the existing sender remains. The user would see this error in the node's status.

## Parameters

- **Value** (0..1, default 1.0): Linear gain multiplier applied when copying audio to the shared buffer.

## CPU Assessment

baseline: low
polyphonic: false
scalingFactors: []

## Notes

- `reset()` calls `currentSlot->clearSignal()` which zeros the shared audio buffer.
- `isSource()` returns true (distinguishes send from receive in the base class).
- `getGain()` returns a constant 1.0f (used for UI metering display, not for actual gain -- the actual gain comes from `value`).
