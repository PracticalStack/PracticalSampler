# control.delay_cable - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/CableNodes.h:2362`
**Base class:** `multi_parameter<NV, ParameterType, multilogic::delay_cable>` (via `pimpl::no_mod_normalisation`)
**Classification:** control_source

## Signal Path

Value parameter input -> store value and start wait counter -> process()/processFrame() counts samples -> after DelayTimeSamples elapsed, dirty flag fires -> sendPending() calls output parameter with stored value.

The delay_cable is a multi_parameter type using `multilogic::delay_cable` as the DataType. When Value changes, the value is stored, `uptime` resets to 0, `wait` is set to true, and `dirty` is cleared. During audio processing (process/processFrame), `uptime` increments by sample count. When `uptime >= delayTimeSamples`, `wait` becomes false and `dirty` becomes true, causing `sendPending()` to forward the stored value to the output.

## Gap Answers

### delay-mechanism: How does the delay buffer work internally?

There is no ring buffer. The mechanism is a simple sample counter: when Value changes, `uptime` resets to 0 and `wait` is set to true. In `process()`, `uptime += numSamples`; in `processFrame()`, `++uptime`. When `uptime >= delayTimeSamples`, the `dirty` flag is set and the value is forwarded on the next `sendPending()` call. When `DelayTimeSamples` is 0, the condition `uptime >= 0` is immediately true on the next process call (not in the same setValue call), so there is still a minimum one-block delay.

### delay-time-modulation: Can DelayTimeSamples be modulated in real-time?

Yes. `setParameter<1>` directly sets `delayTimeSamples = v`. Changing delay time affects the currently waiting value because the comparison `uptime >= delayTimeSamples` uses the current value. If delay time is shortened below the current uptime, the value fires on the next process call. If lengthened, it extends the wait.

### multilogic-delay-cable: What does multilogic::delay_cable do? Does needsProcessing() return true?

Yes, `needsProcessing()` returns `true`. This is critical -- it means `multi_parameter::process()` and `processFrame()` forward to the DataType's process/processFrame methods, which count samples. This node DOES run during audio processing, unlike most control nodes. `getValue()` simply returns the stored value and clears dirty.

## Parameters

- **Value**: The value to delay. When set, resets uptime to 0 and starts waiting. Marked as unscaled input.
- **DelayTimeSamples**: Number of samples to wait before forwarding the value. Range 0-44100. Takes effect immediately on currently queued values.

## CPU Assessment

baseline: negligible
polyphonic: true
scalingFactors: []

## Notes

Because `needsProcessing()` is true, delay_cable must be placed inside the signal path (unlike most control nodes). The `multi_parameter` template forwards process/processFrame to the DataType when this flag is true. Only one value can be "in flight" at a time -- setting Value again while waiting resets the counter.
