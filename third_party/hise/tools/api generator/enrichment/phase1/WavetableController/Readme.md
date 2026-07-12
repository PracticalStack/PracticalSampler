# WavetableController -- Class Analysis

## Brief
Script handle for wavetable synth resynthesis, post-FX processing, caching, and export.

## Purpose
WavetableController provides scripting access to a WavetableSynth module's resynthesis pipeline. It allows loading audio data from files or buffers, configuring resynthesis options (phase modes, cycle detection, denoising), applying post-FX processing chains to wavetable cycles, and exporting results as HWT or WAV files. The controller also supports a resynthesis cache that stores processed wavetables to disk for reuse when the same source file and options are loaded again.

## Details

### Resynthesis Options

The resynthesis pipeline is configured via a JSON object. See `setResynthesisOptions` for the full property reference and `getResynthesisOptions` for retrieving current values.

Key properties: `PhaseMode` (phase alignment mode), `NumCycles` (fixed cycle count or -1 for auto), `RemoveNoise` (SiTraNo noise separation), `UseLoris` (Loris-based resynthesis, requires `HISE_INCLUDE_LORIS`), `RootNote` (pitch detection override). Call `resynthesise` after configuring options.

### Post-FX Processing

Post-FX processors are applied to individual wavetable cycles after resynthesis. Each processor receives a normalized cycle index (0.0 to 1.0) that can optionally pass through a Table lookup for additional shaping. The parameter value is then mapped through the configured range and applied to the cycle waveform. See `setPostFXProcessors` for the full list of processor types and configuration properties.

### Data Loading

See `loadData` for accepted input formats (ScriptFile, Array of Buffers, or single Buffer) and parameter requirements. Call `resynthesise` after loading data to trigger wavetable generation.

### Export Formats

Two export formats are available via `saveAsHwt` (binary ValueTree format loaded directly by WavetableSynth) and `saveAsAudioFile` (48kHz/24-bit WAV with loop point metadata).

## obtainedVia
`Synth.getWavetableController(processorId)` -- processorId must reference a WavetableSynth module.

## minimalObjectToken
wc

## Constants
None.

## Dynamic Constants
None.

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| `wc.loadData(file, 44100, [0, 1000]); wc.resynthesise();` called immediately | Load data, configure options with `setResynthesisOptions()`, then call `resynthesise()` | Resynthesis uses the current options -- configure them before triggering |

## codeExample
```javascript
// Get a reference to a WavetableSynth module
const var wc = Synth.getWavetableController("WavetableSynth1");

// Configure resynthesis
var options = wc.getResynthesisOptions();
options.PhaseMode = "StaticPhase";
options.NumCycles = 64;
wc.setResynthesisOptions(options);
```

## Alternatives
Sampler -- for traditional sample-based playback with key/velocity mapping instead of wavetable morphing.

## Related Preprocessors
`HISE_INCLUDE_LORIS` -- enables Loris-based resynthesis (affects `UseLoris` option availability).

## Diagrams
None.

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: Methods are straightforward delegations to WavetableSynth with no silent-failure preconditions or timeline dependencies beyond the standard null-synth check.
