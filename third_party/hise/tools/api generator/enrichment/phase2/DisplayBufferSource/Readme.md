# DisplayBufferSource -- Project Context

## Project Context

### Real-World Use Cases
- **EQ spectrum analyser**: An FX plugin that displays real-time FFT spectrum data from a parametric EQ module. The DisplayBufferSource retrieves the EQ's display buffer, which is then configured with FFT properties (window type, decibel range, decay) and visualized as a filled path in a ScriptPanel timer callback.
- **Compressor/limiter gain reduction meter**: A dynamics processor display that reads two display buffers from the same processor - gain reduction at index 0 and peak level at index 1 - to draw an interactive threshold control overlaid with real-time level visualization.
- **Multi-processor batch visualization**: A channel strip architecture where each channel has its own EQ or dynamics processor. DisplayBufferSource objects are created in a loop for every channel, and the resulting DisplayBuffer references are stored in an array for indexed access when switching between channels.

### Complexity Tiers
1. **Single buffer visualization** (most common): `Synth.getDisplayBufferSource(id).getDisplayBuffer(0)` chained directly, followed by `createPath()` in a timer callback. Covers simple gain reduction meters and basic FFT displays.
2. **Multi-buffer dynamics display**: Access multiple buffer indices from the same processor (e.g., gain reduction and peak). Store the DisplayBufferSource in a local variable, then call `getDisplayBuffer()` with different indices. Pairs with `getReadBuffer()` for raw sample access.
3. **Batch multi-channel setup**: Loop over an array of processor IDs, create a DisplayBufferSource for each, collect the DisplayBuffer references into an array. Enables switching the active visualization when the user selects a different channel.

### Practical Defaults
- Chain `getDisplayBufferSource(id).getDisplayBuffer(0)` directly when you only need one buffer from a processor - no need to store the intermediate DisplayBufferSource object.
- Store the DisplayBufferSource in a local variable when you need multiple buffer indices from the same processor.
- Dynamics processors (compressor, limiter) typically expose gain reduction at index 0 and peak at index 1.
- A timer interval of 30ms is a good default for dynamics meters; EQ spectrum displays can use the same or slightly faster intervals.

### Integration Patterns
- `Synth.getDisplayBufferSource(id)` -> `DisplayBufferSource.getDisplayBuffer(index)` -- the two-step access pattern; always in onInit.
- `DisplayBuffer.setRingBufferProperties({...})` -- configure FFT parameters immediately after obtaining the buffer.
- `DisplayBuffer.createPath(bounds, range, startValue)` -> `Graphics.fillPath()` -- the standard visualization pipeline inside a ScriptPanel timer callback.
- `DisplayBuffer.getReadBuffer()` -- alternative to `createPath()` when you need raw sample values for custom drawing logic.
- `DisplayBuffer.setActive(value)` -- toggle visualization on/off to save CPU when the display is not visible.

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| Creating DisplayBufferSource inside a timer or control callback | Create in onInit, store in `const var` | `Synth.getDisplayBufferSource()` is onInit-only. The returned handle is valid for the lifetime of the script. |
| Chaining `.getDisplayBuffer(0).getDisplayBuffer(1)` to get two buffers | Store source in a variable, call `getDisplayBuffer()` twice | `getDisplayBuffer()` returns a DisplayBuffer, not another DisplayBufferSource. Call it on the source object for each index. |
| Forgetting to call `setRingBufferProperties()` for FFT display | Configure FFT properties immediately after obtaining the buffer | Without explicit configuration, the buffer uses default settings that may not match your EQ's frequency range or resolution needs. |
