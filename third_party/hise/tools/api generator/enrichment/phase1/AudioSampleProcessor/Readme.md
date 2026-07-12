# AudioSampleProcessor -- Class Analysis

## Brief
Script handle to a module with audio file playback, providing file loading, sample range, and loop control.

## Purpose
The `AudioSampleProcessor` is a script handle to any processor module that owns an audio file slot (AudioLoopPlayer, Convolution Reverb, Wavetable Synthesiser, Noise Grain Player). It provides methods to load audio files from the pool, control the active sample range and loop range, query sample lengths, and access the underlying `AudioFile` complex data object. It also exposes the standard module attribute get/set and bypass interface shared with other module handle classes (Effect, Modulator, ChildSynth).

## Details

### Wrapped Processor Types

The handle wraps any `ProcessorWithExternalData` that has at least one `AudioFile` data slot. Known implementing types:

| Module Type | Processor ID | Base Class | Primary Use |
|-------------|-------------|------------|-------------|
| Audio Loop Player | AudioLooper | ModulatorSynth | Single-file playback with loop, pitch tracking, tempo sync |
| Convolution Reverb | Convolution | MasterEffectProcessor | Impulse response convolution |
| Wavetable Synthesiser | WavetableSynth | ModulatorSynth | Wavetable data source |
| Noise Grain Player | NoiseGrainPlayer | VoiceEffectProcessor | Granular noise residual playback |

### Dynamic Constants

The constructor dynamically adds the wrapped processor's parameter names as integer constants (via `addConstant()`). These vary by module type -- an AudioLooper exposes `SyncMode`, `LoopEnabled`, `PitchTracking`, `RootNote`, `SampleStartMod`, `Reversed` (plus inherited `Gain`, `Balance`, `VoiceLimit`, `KillFadeTime`), while a ConvolutionEffect exposes `DryGain`, `WetGain`, `Predelay`, `HiCut`, `Damping`, etc.

### File Reference Format

File paths use HISE pool reference strings, not filesystem paths. The `{PROJECT_FOLDER}` wildcard resolves to the project's AudioFiles folder. Expansion files use `{EXP::expansionName}` syntax. See `setFile()` and `getFilename()` for usage details.

### Audio File Slot Model

All AudioSampleProcessor instances manage exactly one audio file slot (slot 0). See `getAudioFile()` for the full AudioFile access API, including sample data, callbacks, and direct buffer manipulation.

## obtainedVia
`Synth.getAudioSampleProcessor(processorId)`

## minimalObjectToken
asp

## Constants
(None -- constants are dynamic, added per-instance based on the wrapped processor's parameters.)

## Dynamic Constants
| Name | Type | Description |
|------|------|-------------|
| (per-parameter) | Integer | Parameter index constants from the wrapped processor. Names and count vary by module type. Use `getNumAttributes()` and `getAttributeId(index)` to discover available parameters at runtime. |

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| `asp.setFile("C:/audio/file.wav");` | `asp.setFile("{PROJECT_FOLDER}file.wav");` | setFile expects a pool reference string with wildcards, not an absolute filesystem path. |
| Calling `setFile` in HISE IDE without loading pool | Call `Engine.loadAudioFilesIntoPool()` before `setFile` | In the backend (HISE IDE), audio files must be loaded into the pool first. Exported plugins have embedded files and do not need this step. |

## codeExample
```javascript
// Get a reference to an Audio Loop Player module
const var asp = Synth.getAudioSampleProcessor("AudioLooper1");

// Load an audio file from the project pool
asp.setFile("{PROJECT_FOLDER}my_loop.wav");

// Set the playback range (in samples)
asp.setSampleRange(0, asp.getTotalLengthInSamples());
```

## Alternatives
- **Sampler**: Use Sampler for multi-sample instruments with key/velocity mapping and round-robin groups. AudioSampleProcessor plays a single audio file with sample range control.

## Related Preprocessors
`USE_BACKEND` -- pool loading validation in `setFile()` only applies in the HISE IDE.

## Diagrams

### complex-audio-data-chain
- **Brief:** Audio File Data Chain
- **Type:** topology
- **Description:** Audio file workflows use a three-part chain. `AudioSampleProcessor` selects the processor that owns one or more audio file slots, `AudioFile` exposes the complex data stored in a specific slot, and `ScriptAudioWaveform` displays or edits that same slot in the UI. The binding pair is `processorId` plus `sampleIndex`, which is a complex-data connection and not the normal `parameterId` parameter binding path.

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: AudioSampleProcessor is a straightforward handle with no timeline dependencies or silent-failure preconditions beyond the standard object validity check.
