# DisplayBufferSource -- Method Documentation

## getDisplayBuffer

**Signature:** `ScriptObject getDisplayBuffer(int index)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new ScriptRingBuffer (DisplayBuffer) object on each call.
**Minimal Example:** `var buf = {obj}.getDisplayBuffer(0);`

**Description:**
Returns a DisplayBuffer reference for the display buffer at the given index on the source processor. The returned DisplayBuffer object provides methods for reading buffer contents, creating visualization paths, and configuring buffer properties. Each call creates a new wrapper object pointing to the same underlying ring buffer data.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| index | Integer | no | Zero-based index of the display buffer slot on the source processor. | 0 to numDisplayBuffers-1 |

**Pitfalls:**
- [BUG] If the source processor has been deleted (weak reference expired), the method silently returns an invalid object without reporting an error. Subsequent calls on the returned object will fail.

**Cross References:**
- `$API.Synth.getDisplayBufferSource$`
- `$API.DisplayBuffer.getReadBuffer$`
- `$API.DisplayBuffer.createPath$`
