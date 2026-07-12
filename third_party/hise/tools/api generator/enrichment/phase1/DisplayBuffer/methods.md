# DisplayBuffer -- Method Entries

## copyReadBuffer

**Signature:** `undefined copyReadBuffer(AudioData targetBuffer)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Acquires SimpleReadWriteLock read lock on data lock, then CriticalSection on read buffer lock for the copy operation.
**Minimal Example:** `{obj}.copyReadBuffer(myBuffer);`

**Description:**
Copies the ring buffer's read data into a preallocated target buffer. Accepts two input forms: a single Buffer copies channel 0, or an Array of Buffers copies per-channel. The target buffer size must match the ring buffer's sample count exactly, and for the array form the array length must match the channel count. This is the thread-safe way to obtain ring buffer data for processing -- unlike `getReadBuffer()`, it returns an independent copy.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| targetBuffer | AudioData | no | Pre-allocated destination -- a single Buffer (channel 0) or an Array of Buffers (multi-channel) | Buffer size must match read buffer sample count; array length must match channel count |

**Cross References:**
- `$API.DisplayBuffer.getReadBuffer$`
- `$API.DisplayBuffer.getResizedBuffer$`
- `$API.DisplayBuffer.createPath$`

## createPath

**Signature:** `ScriptObject createPath(Array dstArea, Array sourceRange, Number normalisedStartValue)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a PathObject and acquires SimpleReadWriteLock read lock on the buffer data lock.
**Minimal Example:** `var p = {obj}.createPath([0, 0, 500, 200], [-1.0, 1.0, 0, -1], 0.0);`

**Description:**
Creates a Path object from the ring buffer data, scaled to the given destination rectangle. Path generation is delegated to the active PropertyObject, so the visual result depends on the buffer source type (oscilloscope, FFT, envelope, etc.). The `sourceRange` parameter encodes both the value range and sample range as a rectangle: `[minValue, maxValue, startSample, endSample]`. The `normalisedStartValue` is passed through to the PropertyObject's path generator as the initial path position.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| dstArea | Array | no | Destination rectangle [x, y, width, height] in pixels | 4-element array parseable as rectangle |
| sourceRange | Array | no | Source range as [minValue, maxValue, startSample, endSample] | minValue clamped to >= -1.0, maxValue clamped to <= 1.0, startSample clamped to >= 0 |
| normalisedStartValue | Number | no | Initial path position passed to the PropertyObject path generator | -- |

**Pitfalls:**
- [BUG] The sourceRange endSample (fourth element) is ignored due to a variable overwrite in the C++ implementation (line ~1913). The path always renders the full buffer length regardless of the endSample value. Only the startSample (third element) is respected.
- The sourceRange uses a non-standard packing: `[minValue, maxValue, startSample, endSample]`. This is not a spatial rectangle -- the first two elements define the value normalization range and the last two define the sample range.

**Cross References:**
- `$API.DisplayBuffer.getReadBuffer$`
- `$API.DisplayBuffer.setRingBufferProperties$`

**Example:**
```javascript:createpath-panel-visualization
// Title: Render a display buffer waveform in a ScriptPanel
const var db = Engine.createAndRegisterRingBuffer(0);
const var panel = Content.addPanel("WaveformDisplay", 0, 0);
panel.set("width", 500);
panel.set("height", 200);

panel.setPaintRoutine(function(g)
{
    g.fillAll(0xFF222222);

    // sourceRange: value range [-1, 1], full buffer from sample 0
    var p = db.createPath([0, 0, this.getWidth(), this.getHeight()],
                          [-1.0, 1.0, 0, -1], 0.0);
    g.setColour(Colours.lime);
    g.drawPath(p, {}, 1.0);
});

panel.setTimerCallback(function()
{
    this.repaint();
});

panel.startTimer(30);
```
```json:testMetadata:createpath-panel-visualization
{
  "testable": false,
  "skipReason": "Requires visual verification of rendered path in panel paint routine"
}
```

## fromBase64

**Signature:** `undefined fromBase64(String b64, Integer useUndoManager)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** String parameter, potential UndoableAction allocation and undo manager transaction when useUndoManager is true.
**Minimal Example:** `{obj}.fromBase64(savedState, false);`

**Description:**
Restores the display buffer state from a base64-encoded string. The restore operation is delegated to the active PropertyObject -- only buffer types that implement state serialization (e.g., flex AHDSR envelopes) produce meaningful results. When `useUndoManager` is true, the operation is wrapped in an UndoableAction that captures the previous state for undo/redo support via the control undo manager.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| b64 | String | no | Base64-encoded state string from toBase64() | -- |
| useUndoManager | Integer | no | If true, wraps the restore in an undoable action | 0 or 1 |

**Pitfalls:**
- Silently does nothing if the active PropertyObject does not implement `restoreFromBase64()`. Most buffer types (FFT, oscilloscope, goniometer, generic) do not support state serialization -- only envelope types (flex AHDSR) implement it.

**Cross References:**
- `$API.DisplayBuffer.toBase64$`

## getReadBuffer

**Signature:** `Buffer getReadBuffer()`
**Return Type:** `Buffer`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new VariantBuffer wrapper object on the heap.
**Minimal Example:** `var buf = {obj}.getReadBuffer();`

**Description:**
Returns a Buffer wrapping channel 0 of the internal read buffer. The returned Buffer shares memory with the ring buffer -- it is a direct pointer reference, not a copy. This is useful for quick size checks or read-only inspection, but modifying the returned buffer corrupts the shared ring buffer data.

**Parameters:**
None.

**Pitfalls:**
- The returned Buffer is a direct memory reference to the ring buffer's internal read buffer, not a copy. Writing to the returned buffer corrupts the ring buffer data visible to other consumers. Use `copyReadBuffer()` for a safe independent copy.

**Cross References:**
- `$API.DisplayBuffer.copyReadBuffer$`
- `$API.DisplayBuffer.getResizedBuffer$`

## getResizedBuffer

**Signature:** `Buffer getResizedBuffer(Integer numDestSamples, Integer resampleMode)`
**Return Type:** `Buffer`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new VariantBuffer on the heap. When source and destination sizes match, delegates to getReadBuffer() which also allocates.
**Minimal Example:** `var buf = {obj}.getResizedBuffer(512, 0);`

**Description:**
Creates a new Buffer resampled from the read buffer to the specified number of samples. When the stride is less than 2.0 (upsampling or slight downsampling), uses point sampling. When the stride is 2.0 or greater (significant downsampling), finds the min/max in each stride window and takes the midpoint. Returns an empty zero-length buffer if `numDestSamples` is 0 or negative.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| numDestSamples | Integer | no | Target buffer size in samples | Must be > 0 for useful output |
| resampleMode | Integer | no | Intended resample algorithm selector | -- |

**Pitfalls:**
- [BUG] The `resampleMode` parameter is accepted but completely ignored. The resampling algorithm is determined solely by the stride ratio (source size / destination size), not by this parameter.
- [BUG] When `numDestSamples` matches the read buffer size exactly, the method delegates to `getReadBuffer()`, returning a shared memory reference instead of an independent copy. For all other sizes, an independent buffer is returned. This inconsistency means the returned buffer may or may not be safe to modify depending on the size relationship.

**Cross References:**
- `$API.DisplayBuffer.getReadBuffer$`
- `$API.DisplayBuffer.copyReadBuffer$`

## setActive

**Signature:** `undefined setActive(Integer shouldBeActive)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setActive(false);`

**Description:**
Enables or disables the ring buffer. When disabled, the DSP writer skips writing to the ring buffer, reducing CPU overhead for display buffers that are not currently visible. Re-enabling resumes writing from the current position.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shouldBeActive | Integer | no | true (1) to enable writing, false (0) to disable | -- |

**Cross References:**
- `$API.DisplayBuffer.setRingBufferProperties$`

## setRingBufferProperties

**Signature:** `undefined setRingBufferProperties(JSON propertyData)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Delegates to PropertyObject::setProperty() which may trigger buffer resizing or lock acquisition.
**Minimal Example:** `{obj}.setRingBufferProperties({"BufferLength": 8192});`

**Description:**
Configures the ring buffer by passing a JSON object of key-value pairs to the active PropertyObject. The available properties depend on the buffer source type -- each DSP node that writes to a display buffer registers its own PropertyObject with type-specific properties. Common properties include `BufferLength` and `NumChannels`. Unknown keys are silently ignored.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| propertyData | JSON | no | Object with property key-value pairs | Keys must match the active PropertyObject's supported properties |

**Pitfalls:**
- Unknown property keys are silently ignored. There is no error or warning when a key does not match any property supported by the active PropertyObject. Typos in property names (e.g., "bufferLength" instead of "BufferLength") produce no feedback.

**Cross References:**
- `$API.DisplayBuffer.createPath$`
- `$API.DisplayBuffer.setActive$`

**Example:**
```javascript:configure-fft-properties
// Title: Configure FFT analyser display buffer properties
const var src = Synth.getDisplayBufferSource("Analyser1");
const var db = src.getDisplayBuffer(0);

// FFT-specific properties (only available when source is an FFT analyser)
db.setRingBufferProperties({
    "BufferLength": 8192,
    "WindowType": "BlackmanHarris",
    "Overlap": 2,
    "UseDecibelScale": true,
    "DecibelRange": [-80, 0],
    "UsePeakDecay": true,
    "Decay": 0.95
});
```
```json:testMetadata:configure-fft-properties
{
  "testable": false,
  "skipReason": "Requires an FFT analyser module with a connected display buffer source"
}
```

## toBase64

**Signature:** `String toBase64()`
**Return Type:** `String`
**Call Scope:** unsafe
**Call Scope Note:** String construction from PropertyObject::exportAsBase64() involves heap allocation.
**Minimal Example:** `var state = {obj}.toBase64();`

**Description:**
Exports the display buffer state as a base64-encoded string. The export is delegated to the active PropertyObject -- only buffer types that implement state serialization produce non-empty output. The returned string can be passed to `fromBase64()` to restore the state.

**Parameters:**
None.

**Pitfalls:**
- Returns an empty string without error if the active PropertyObject does not implement `exportAsBase64()`. Most buffer types (FFT, oscilloscope, goniometer, generic) do not support state serialization -- only envelope types (flex AHDSR) implement it. There is no way to distinguish "empty state" from "unsupported operation".

**Cross References:**
- `$API.DisplayBuffer.fromBase64$`
