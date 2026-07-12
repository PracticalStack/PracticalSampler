# routing.global_receive - C++ Exploration

**Source:** `hi_scripting/scripting/scriptnode/dynamic_elements/GlobalRoutingManager.cpp:1577`
**Base class:** `GlobalRoutingNodeBase`
**Classification:** audio_processor

## Signal Path

Adds audio from a shared GlobalRoutingManager Signal buffer into the local signal chain. The received audio is ADDED to the existing signal (not replacing it), scaled by the Value parameter.

`process()`: acquires `ScopedTryReadLock(connectionLock)`, checks slot is valid, specs match, and node is not bypassed, then calls `currentSlot->pop(data, value.get(), offset.get())`.

`Signal::pop()` uses `FloatVectorOperations::addWithMultiply(data[i], channelData[i] + offset, value, numSamples)` -- this mixes the shared buffer content into the existing output signal.

## Gap Answers

### signal-add-not-replace: From infrastructure: pop() uses addWithMultiply, meaning the received signal is ADDED to the existing signal at the receive point. Confirm this -- does the node mix into whatever signal is already flowing, or replace it?

Confirmed: additive mixing. `Signal::pop()` uses `FloatVectorOperations::addWithMultiply()`, which adds gain-scaled samples from the shared buffer on top of whatever signal is already in the ProcessDataDyn channels. If you want the received signal alone, the receive node should be placed after a point where the signal is silent (e.g., in a separate chain branch or after a gain of 0).

### spec-matching-requirements: The infrastructure docs state receivers must match sender specs (sample rate, channel count, blockSize <= sender). What error is shown to the user when there is a mismatch?

In `process()`, the node calls `currentSlot->matchesSourceSpecs(lastSpecs)` and checks if `.error == Error::OK` before proceeding. If it does not match, pop() is simply not called -- no audio is received. The `Signal::matchesSourceSpecs()` method returns specific error types:
- `Error::SampleRateMismatch` if sample rates differ
- `Error::ChannelMismatch` if channel counts differ  
- `Error::BlockSizeMismatch` if receiver blockSize > sender blockSize

These errors are stored in the node's `lastResult` for UI display.

### description-inaccuracy: The description says 'Send the signal anywhere in HISE!' but this is the RECEIVE node.

Confirmed copy-paste error. `getNodeDescription()` at line 1642 returns "Send the signal anywhere in HISE!" which is identical to GlobalSendNode. The correct description should be: "Receive a signal sent from a global_send node anywhere in HISE."

### polyphonic-offset-handling: The infrastructure mentions GlobalReceiveNode is templated on NV for polyphonic voice count, with per-voice offset tracking. But cppProperties does not list IsPolyphonic. Is this node actually polyphonic or monophonic?

The node IS polyphonic. `GlobalReceiveNode<NV>` is registered with `registerPolyNodeRaw<GlobalReceiveNode<1>, GlobalReceiveNode<NUM_POLYPHONIC_VOICES>>()`. It has `PolyData<float, NumVoices> value` and `PolyData<int, NumVoices> offset` members. The `handleHiseEvent()` method computes per-voice offset on note-on when `NumVoices > 1`. The missing `IsPolyphonic` property in cppProperties is because the polyphonic registration is handled by the `registerPolyNodeRaw` factory system rather than a CustomNodeProperty. The base data classification showing `isPolyphonic: false` is incorrect for the polyphonic variant.

## Polyphonic Behaviour

- `PolyData<float, NumVoices> value` -- per-voice gain value (set via parameter callback which iterates all voices)
- `PolyData<int, NumVoices> offset` -- per-voice read offset into the shared buffer
- On note-on: `offset.get()` is set to `roundToInt(startStamp * ratio)` where `ratio = lastSpecs.sampleRate / mainSynthChain->getSampleRate()`. This aligns the read position for sub-block voice starts.
- If source and receiver block sizes match, offset is effectively the event timestamp scaled by sample rate ratio.
- `reset()` zeros all voice offsets.

## Parameters

- **Value** (0..1, default 1.0): Linear gain multiplier applied when adding audio from the shared buffer into the local signal.

## CPU Assessment

baseline: low
polyphonic: true
scalingFactors: []

## Notes

- Only processes audio when specs match the sender and the slot has a valid source.
- The additive mixing means multiple global_receive nodes can pull from the same slot and each adds independently.
- The per-voice offset system handles the case where polyphonic voices start at different sub-block positions, ensuring correct alignment with the monophonic shared buffer.
