<!-- Diagram triage:
  - No diagrams specified in Phase 1 data
-->

# DisplayBuffer

DisplayBuffer provides script-level access to a ring buffer owned by a DSP processor, enabling real-time audio visualisation in your UI. Many scriptnode nodes produce visualisation data - oscillators have waveforms, envelopes have curves, and analysers have frequency spectra. By default these are only visible inside the scriptnode editor, but you can expose them to your scripted interface via the external data slot system.

To connect a DisplayBuffer to your UI:

1. Assign an **External Data slot** to the node in the scriptnode editor (click the plot icon and choose a free slot).
2. Obtain a typed reference via `Synth.getDisplayBufferSource()` and `DisplayBufferSource.getDisplayBuffer()`.
3. Use `createPath()` inside a timer callback to generate a drawable path, then render it in a ScriptPanel paint routine.

```js
const var src = Synth.getDisplayBufferSource("Analyser1");
const var db = src.getDisplayBuffer(0);
```

The buffer's behaviour adapts automatically to its source type. An FFT analyser buffer accepts properties like `"WindowType"` and `"DecibelRange"`, while an oscilloscope buffer accepts `"BufferLength"` and `"NumChannels"`. Use `setRingBufferProperties()` to configure the active buffer type.

| Source Type | Typical Properties |
|---|---|
| FFT Analyser | BufferLength, WindowType, Overlap, DecibelRange, UsePeakDecay, UseDecibelScale, YGamma, Decay, UseLogarithmicFreqAxis |
| Oscilloscope | BufferLength (128-65536), NumChannels (1-2) |
| Goniometer | BufferLength (512-32768), NumChannels (fixed 2) |
| Generic | BufferLength, NumChannels, Active |
| Envelope / Oscillator | Fixed size, internal display |

> DisplayBuffer references can only be created during `onInit`. Each external data slot uses a single-writer rule - you cannot assign multiple nodes to the same slot. Call `createPath()` in the timer callback rather than the paint routine, since it acquires a lock on the ring buffer data. Use `setActive(false)` on buffers that are not currently visible to save CPU.

## Common Mistakes

- **Use copyReadBuffer for writable data**
  **Wrong:** `var buf = db.getReadBuffer(); buf[0] = 1.0;`
  **Right:** `var buf = Buffer.create(size); db.copyReadBuffer(buf);`
  *`getReadBuffer()` returns a direct reference to shared memory. Writing to it corrupts the ring buffer. Use `copyReadBuffer()` for an independent copy.*

- **Match buffer sizes for copyReadBuffer**
  **Wrong:** `db.copyReadBuffer(Buffer.create(512))` when the ring buffer holds 8192 samples
  **Right:** `db.copyReadBuffer(Buffer.create(8192))`
  *The target buffer size must match exactly. Check the read buffer length first, or use `getResizedBuffer()` to resample to a different size.*

- **Deactivate invisible buffers**
  **Wrong:** Leaving all display buffers active regardless of which page is shown
  **Right:** Call `setActive(false)` on buffers for hidden pages, `setActive(true)` when switching back
  *FFT analysis runs on the audio thread even when the UI is not rendering the result. Disabling invisible buffers saves CPU.*

- **Generate paths in the timer, not the paint routine**
  **Wrong:** Calling `createPath()` inside `setPaintRoutine`
  **Right:** Call `createPath()` in `setTimerCallback`, store the path, then draw it in the paint routine
  *`createPath()` acquires a read lock on the ring buffer. Calling it in the paint routine risks blocking the UI render thread.*
