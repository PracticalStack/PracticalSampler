<!-- Diagram triage:
  - No diagrams specified in Phase 1
-->
# DisplayBufferSource

DisplayBufferSource is a handle to a processor that owns one or more display buffers - ring buffers used for real-time waveform and spectrum visualisation. It acts as an intermediary in a two-step access pattern: first obtain a reference to the source processor, then retrieve individual display buffers by index.

Two categories of HISE modules can act as display buffer sources:

- **Built-in processors with static display buffers** - modules such as the Analyser node that always expose a display buffer as part of their signal processing.
- **Script modules** - all script modules can host multiple display buffers, making them flexible containers for custom visualisation data.

To obtain a DisplayBufferSource, call `Synth.getDisplayBufferSource()` with the processor ID in your `onInit` callback:

```javascript
const var dbs = Synth.getDisplayBufferSource("MyProcessor");
const var db = dbs.getDisplayBuffer(0);
```

For single-buffer access, you can chain the calls directly. When you need multiple buffers from the same processor (common with dynamics processors that expose gain reduction and peak level on separate indices), store the DisplayBufferSource in a variable and call `getDisplayBuffer()` for each index.

> DisplayBufferSource objects must be created in `onInit`. The factory method is not available in control callbacks, timer callbacks, or other runtime contexts.

## Common Mistakes

- **Wrong:** `var dbs = Synth.getDisplayBufferSource("MyEffect"); // in onControl`
  **Right:** `const var dbs = Synth.getDisplayBufferSource("MyEffect"); // in onInit`
  *The factory method only works during initialisation. Calling it outside `onInit` throws a script error.*

- **Wrong:** Chaining `.getDisplayBuffer(0).getDisplayBuffer(1)` to get two buffers
  **Right:** Store the source, then call `getDisplayBuffer()` separately for each index
  *`getDisplayBuffer()` returns a DisplayBuffer, not another DisplayBufferSource. Call it on the source object for each buffer index you need.*

- **Wrong:** Displaying an FFT spectrum without calling `setRingBufferProperties()` first
  **Right:** Configure FFT properties (buffer length, window type, decibel range) immediately after obtaining the buffer
  *Without explicit configuration, the buffer uses default settings that may not match your processor's frequency range or resolution requirements.*

**See also:** $SN.core.peak$ -- Scriptnode peak meter node whose display buffer can be accessed through this API
