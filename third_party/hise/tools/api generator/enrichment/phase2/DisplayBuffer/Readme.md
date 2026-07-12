# DisplayBuffer -- Project Context

## Project Context

### Real-World Use Cases
- **FFT spectrum analyser for EQ modules**: A plugin with a parametric EQ uses DisplayBuffer to visualize the frequency spectrum behind the EQ curve. The buffer is obtained from the EQ module via `Synth.getDisplayBufferSource()`, configured with FFT-specific properties (window type, decibel range, logarithmic axis), and rendered inside a FloatingTile's DraggableFilterPanel or a custom ScriptPanel.
- **Compressor/limiter gain reduction meter**: A dynamics processor plugin uses two display buffers per compressor -- one for peak level (index 1) and one for gain reduction (index 0). A ScriptPanel timer reads the buffers via `createPath()` to render real-time level and gain reduction curves, giving visual feedback of the compression behavior.
- **Multi-channel dynamics display**: A multi-channel plugin obtains display buffers from per-channel compressors and switches the active visualization when the user selects a different channel. Only the currently visible channel's buffer is active to save CPU.

### Complexity Tiers
1. **Basic visualization** (most common): `getDisplayBuffer()` + `setRingBufferProperties()` + `createPath()` in a timer. Sufficient for a single oscilloscope, FFT overlay, or gain reduction meter.
2. **Multi-buffer management**: Multiple display buffers from different processors, shared property configurations, `setActive()` to disable invisible buffers. Used when a plugin has per-channel EQs or multiple dynamics processors.
3. **Direct buffer access**: `getReadBuffer()` for custom processing of buffer data (magnitude calculation, encoding for WebView transfer). Used when `createPath()` is not flexible enough for the visualization.

### Practical Defaults
- Use a timer interval of 30ms (`startTimer(30)`) for smooth display updates -- this gives approximately 33 fps which is sufficient for audio visualization without excessive CPU use.
- For FFT spectrum analysis, `"BufferLength": 4096`, `"WindowType": "Flat Top"`, `"UseDecibelScale": true`, and `"UseLogarithmicFreqAxis": true` are good starting defaults.
- Use `setActive(false)` on display buffers that are not currently visible. FFT analysis in particular consumes CPU even when the result is not rendered.
- For gain reduction / peak meters, use `createPath()` with `sourceRange` `[0.0, 1.0, 0, -1]` (normalised 0-1 range, full buffer length).

### Integration Patterns
- `Synth.getDisplayBufferSource(moduleId)` -> `DisplayBufferSource.getDisplayBuffer(index)` -- the standard two-step acquisition chain. The source is a processor reference, the index selects which buffer (e.g., 0 = gain reduction, 1 = peak for dynamics processors).
- `DisplayBuffer.createPath()` -> `Graphics.fillPath()` / `Graphics.drawPath()` -- generate a path in a timer callback, render it in the paint routine. Always call `repaint()` after updating the path data.
- `DisplayBuffer.setActive()` -> page/tab switch callback -- tie buffer activation to UI visibility so inactive pages do not waste CPU on FFT or ring buffer processing.
- `DisplayBuffer.createPath()` -> `Path.closeSubPath()` -- close the generated path before using `fillPath()` to create a filled waveform/level display rather than just a stroke.

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| Leaving all display buffers active regardless of which page is shown | Call `setActive(false)` on buffers for hidden pages and `setActive(true)` when switching to their page | FFT analysis runs on the audio thread even when the UI is not rendering the result. Disabling invisible buffers saves CPU. |
| Defining FFT properties inline for each of several EQ buffers | Define a shared properties object (`const var FFT_PROPS = {...}`) and pass it to each buffer's `setRingBufferProperties()` | Reduces duplication and ensures all analysers use consistent settings. |
| Calling `createPath()` in the paint routine | Call `createPath()` in the timer callback, store the path, call `repaint()`, then draw the stored path in the paint routine | `createPath()` acquires a lock on the ring buffer. Calling it in the paint routine risks blocking the UI thread during rendering. |
