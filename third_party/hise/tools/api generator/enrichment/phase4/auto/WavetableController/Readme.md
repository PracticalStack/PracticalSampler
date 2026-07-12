<!-- Diagram triage:
  - No class-level or method-level diagrams specified in Phase 1 data
-->

# WavetableController

WavetableController provides scripting access to a WavetableSynth module's resynthesis pipeline. It handles the complete workflow from loading audio source material through to exporting processed wavetables.

The controller manages three main stages:

1. **Loading** - accepts audio files, single buffers, or multi-channel buffer arrays as source material
2. **Resynthesis** - analyses the audio via FFT (or optionally Loris) and converts it into band-limited wavetable cycles
3. **Post-processing** - applies per-cycle effects (waveshaping, FM, sync, folding) that are baked into the final wavetable

Obtain a reference by passing the WavetableSynth module ID:

```javascript
const var wc = Synth.getWavetableController("WavetableSynth1");
```

The typical workflow is: configure resynthesis options, load audio data, call `resynthesise()`, then optionally export as HWT or WAV. For generated waveforms loaded via `loadData()` with a Buffer, the resynthesis step handles band-limiting automatically through mip-mapping, so aliasing is not a concern.

A resynthesis cache can speed up repeated loads by storing processed results to disk. The cache keys on both the source filename and the current resynthesis options, so changing either invalidates the cached entry.

Two export formats are available: HWT (binary format loaded directly by WavetableSynth) and WAV (48 kHz, 24-bit with loop point metadata for reimport).

> The WavetableSynth is also an AudioSampleProcessor, so the same processor ID works with both `Synth.getWavetableController()` and `Synth.getAudioSampleProcessor()`. Use the WavetableController for resynthesis control and the AudioSampleProcessor interface for direct audio file loading.

## Common Mistakes

- **Wrong:** Calling `resynthesise()` before configuring options
  **Right:** Set options with `setResynthesisOptions()` first, then call `resynthesise()`
  *Resynthesis uses the current options at the time it runs. Loading data and immediately resynthesising skips your intended configuration.*

- **Wrong:** Enabling noise removal for generated waveforms
  **Right:** Set `RemoveNoise = false` and `UseLoris = false` before loading synthetic buffers
  *Noise removal and Loris analysis add processing time and can alter clean synthetic waveforms that have no noise to remove.*

- **Wrong:** Creating a new WavetableController reference for each operation
  **Right:** Store the reference in a `const var` and reuse it
  *`Synth.getWavetableController()` creates a new wrapper object each call. Cache the reference once and reuse it across all operations.*
