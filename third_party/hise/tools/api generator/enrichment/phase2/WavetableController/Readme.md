# WavetableController -- Project Context

## Project Context

### Real-World Use Cases
- **Wavetable synthesizer with user-importable waveforms**: A synth plugin that lets users load audio files, generate basic waveforms (sine, saw, square), and resample between oscillators. The WavetableController manages the resynthesis pipeline while a Broadcaster-driven context menu provides the user-facing operations (clear, load, save, resample, initialize).
- **Cross-oscillator resampling**: A dual-oscillator wavetable synth where one oscillator's rendered output can be captured and loaded into the other oscillator as wavetable data. This uses `Engine.renderAudio()` to capture the output, then `loadData()` with the rendered channel data.

### Complexity Tiers
1. **Basic wavetable loading** (most common): `getResynthesisOptions`, `setResynthesisOptions`, `loadData`. Load audio files or generated buffers into a WavetableSynth and configure resynthesis parameters.
2. **Wavetable export**: Add `saveAsAudioFile` or `saveAsHwt` to export processed wavetables for reuse or distribution.
3. **Advanced pipeline**: Add `setPostFXProcessors` for per-cycle post-processing, `setEnableResynthesisCache` for caching resynthesised data, and `setErrorHandler` for error reporting. Use `Engine.renderAudio()` for cross-oscillator resampling workflows.

### Practical Defaults
- Use `PhaseMode = "StaticPhase"` as the default phase mode - it preserves natural phase relationships without the complexity of dynamic tracking.
- Set `RemoveNoise = false` and `UseLoris = false` when loading single-cycle waveforms or generated buffers - noise removal and Loris analysis are unnecessary for clean synthetic sources.
- Use a buffer size of 2048 samples at 48000 Hz for generated single-cycle waveforms - this provides good frequency resolution and matches the WAV export sample rate.
- Always set `RemoveNoise` explicitly in the options object rather than relying on the default, due to a deserialization bug where it can inherit the value of `ReverseOrder`.

### Integration Patterns
- `Synth.getWavetableController(id)` + `Synth.getAudioSampleProcessor(id).getAudioFile(0)` - A WavetableSynth is accessed through both its WavetableController (for resynthesis control) and its AudioSampleProcessor interface (for audio file loading). The same processor ID works for both.
- `Engine.createBroadcaster()` + `attachToContextMenu()` - Context menus on wavetable display panels drive clear/load/save/resample operations through the WavetableController, keeping UI event handling separate from wavetable logic.
- `Engine.renderAudio()` + `loadData(channels)` - Cross-oscillator resampling captures rendered audio via `Engine.renderAudio()` and feeds the channel data back into a different WavetableController via `loadData()`.
- `FileSystem.browse()` + `saveAsAudioFile()` - File browser dialogs provide the output path for wavetable export.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| Using `loadData` with noise removal enabled for generated waveforms | Set `RemoveNoise = false` and `UseLoris = false` before loading synthetic/generated buffers | Noise removal and Loris analysis add processing time and can alter clean synthetic waveforms unnecessarily |
| Creating one WavetableController per operation | Store the WavetableController reference and reuse it across operations | `Synth.getWavetableController()` creates a new wrapper object each time - cache it in a `const var` or object property |
