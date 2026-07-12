# routing.send - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/RoutingNodes.h:786`
**Base class:** `send_base` (inherits `routing::base` -> `mothernode` + `polyphonic_base`)
**Classification:** audio_processor

## Signal Path

The send node captures audio and stores it in an internal cable buffer. The signal passes through the node unchanged -- it is a tap, not a redirect. In `process()`, the cable's `process()` is called which copies the audio data into the cable's internal buffer via `writeToBuffer()` (for block cables) or stores the frame data (for frame cables). The original signal continues through the chain unmodified since the cable only copies from the ProcessData.

For block-mode cables (`cable::block`), `writeToBuffer()` uses `FloatVectorOperations::copy` to copy the input data into the cable's internal circular buffer. For frame-mode cables (`cable::frame`), `processFrame()` copies each sample from the frame into the cable's `frameData` storage.

## Gap Answers

### signal-copy-or-passthrough: Does routing.send copy the audio buffer to the receive target's buffer, or does it pass a reference? Does the signal continue through the chain after the send node, or is it consumed?

The send node copies the audio data into the cable's internal buffer. The signal is NOT consumed -- it continues through the chain unmodified. The cable's `process()` / `processFrame()` methods only read from the ProcessData and write to internal storage. The original audio buffers are not altered.

For block cables: `cable::block::process()` calls `writeToBuffer()` which uses `FloatVectorOperations::copy` to copy source data to the cable's internal `heap<float>` buffer (line 228-230).

For frame cables: `cable::frame::processFrame()` copies each sample from the frame into `frameData` (line 148-156).

### multiple-targets-behaviour: When Connection targets multiple receive nodes, is the signal copied to each independently? Is there any gain compensation for multiple targets?

The send node has a single `CableType cable` member. The `connect()` method (inherited from `send_base`, line 623) calls `cable->connect(b)` which sets the receive node's `source` pointer to this cable. Multiple receive nodes can point to the same cable object, and each receive node independently reads from the same buffer. There is no gain compensation -- each receive gets the full-level copy. The receive node's Feedback parameter independently controls how much of the signal is mixed in at each target.

### polyphonic-voice-routing: With IsPolyphonic true, does each voice's send independently route to the corresponding voice's receive? Or is the send aggregated across voices?

Voice isolation is maintained. The cable types use `PolyData` for their internal storage: `cable::frame` has `PolyData<span<float, C>, NV> frameData` (line 167) and `cable::block_base` has `PolyData<Data, NV> data` (line 281). During processing, `.get()` returns the current voice's data. Each voice writes to its own storage in the send, and each voice reads from its own storage in the receive.

## CPU Assessment

baseline: negligible
polyphonic: true
scalingFactors: []

## Notes

The send node has no parameters (`createParameters` is empty, line 809). The CableType template parameter determines whether it operates in block mode or frame mode. In scriptnode's runtime, a polymorphic cable type is used; in C++ export, the specific cable type must be specified. The cable also manages read/write indices as a circular buffer for block mode.
