<!-- Diagram triage:
  - (no diagrams in Phase 1 data)
-->

# LorisManager

LorisManager provides a scripting interface to the Loris partial-tracking library for spectral analysis, manipulation, and resynthesis of audio files. It decomposes a sound into individual sinusoidal partials, lets you reshape them, and reconstructs audio from the modified partial list.

The typical workflow follows four stages:

1. **Configure** analysis options with `set()` (optional - the defaults are sensible)
2. **Analyse** an audio file with `analyse()`, passing the estimated root frequency
3. **Manipulate** partials with `process()` (bulk commands) or `processCustom()` (per-breakpoint callback)
4. **Extract** results as audio buffers (`synthesise()`), parameter envelopes (`createEnvelopes()`), display paths (`createEnvelopePaths()`), or harmonic snapshots (`createSnapshot()`)

```javascript
const lm = Engine.getLorisManager();
```

The five partial parameters available for envelope extraction and snapshots are:

| Parameter | Description | Range |
|-----------|-------------|-------|
| `"rootFrequency"` | F0 estimate relative to root, centred on 1.0 | Derived from freqdrift |
| `"frequency"` | Partial frequency in Hz | Derived from freqdrift |
| `"phase"` | Phase in radians | -PI to PI |
| `"gain"` | Amplitude | 0.0 to 1.0 |
| `"bandwidth"` | Noisiness (0 = pure sine, 1 = full noise) | 0.0 to 1.0 |

> All LorisManager methods that accept a `file` parameter require a `File` object (from `FileSystem`). Passing a string path or any other type silently returns false or an empty result without throwing an error.

> Loris operations are computationally heavy. Run analysis and processing on a background thread to keep the UI responsive.

> LorisManager requires the optional `HISE_INCLUDE_LORIS` module to be enabled at compile time.

## Common Mistakes

- **Wrong:** `lm.process(f, "shiftPitch", 2.0)`
  **Right:** `lm.process(f, "shiftPitch", {"offset": 2.0})`
  *Process commands require a JSON object (or array) as the data parameter, not a plain numeric value.*

- **Wrong:** `lm.analyse("path/to/file.wav", 440)`
  **Right:** `lm.analyse(FileSystem.fromAbsolutePath("path/to/file.wav"), 440)`
  *The file parameter must be a File object obtained from FileSystem. A string path silently fails and returns false.*
