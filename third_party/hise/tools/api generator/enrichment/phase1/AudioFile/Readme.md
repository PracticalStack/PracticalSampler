# AudioFile -- Class Analysis

## Brief
Scriptable handle to a processor's audio file slot for loading, range control, and content access.

## Purpose
AudioFile is a script-side reference to a `MultiChannelAudioBuffer` data slot owned by a processor. It provides methods to load audio files from pool references or programmatic buffers, control sample ranges and loop points, query sample properties (length, sample rate), and receive callbacks when content changes or playback position updates. AudioFile is part of the complex data reference system shared with Table, SliderPackData, and DisplayBuffer.

## Details

### Complex Data Reference Architecture

AudioFile inherits from `ScriptComplexDataReferenceBase`, the shared base for all complex data handles. This base class provides:

- **Event listener registration** -- automatically subscribes to the underlying `ComplexDataUIUpdaterBase` for content and display events
- **Callback infrastructure** -- `setDisplayCallback` and `setContentCallback` dispatch through `WeakCallbackHolder` with 1 argument each
- **Data linking** -- `linkTo()` redirects this handle's data slot to share another handle's underlying buffer
- **Holder resolution** -- if no external holder is specified, the script processor itself acts as the data holder

### Two-Layer Buffer Model

The underlying `MultiChannelAudioBuffer` maintains two buffer layers:

| Layer | Access | Description |
|-------|--------|-------------|
| `originalBuffer` | `getTotalLengthInSamples()` | Full audio file as loaded from disk |
| `currentData` | `getNumSamples()`, `getContent()`, `getRange()` | Sub-range extracted via `setRange()` |

After calling `setRange(min, max)`, all data access methods (`getContent()`, `getNumSamples()`) operate on the sub-range, not the original file. Use `getTotalLengthInSamples()` to query the original file length.

### File Loading

Audio data can be loaded from disk via `loadFile()` (HISE pool references) or programmatically via `loadBuffer()` (Buffer objects). See the individual method entries for parameter details and validation rules.

### Callback Events

Two callback types are available: content callbacks (fire on data changes) and display callbacks (fire on playback position updates). See `setContentCallback()` and `setDisplayCallback()` for registration details and callback signatures.

## obtainedVia
`Engine.createAndRegisterAudioFile(index)` or `AudioSampleProcessor.getAudioFile(slotIndex)`

## minimalObjectToken
af

## Constants
None.

## Dynamic Constants
None.

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| `af.getNumSamples()` after `setRange()` expecting full length | `af.getTotalLengthInSamples()` | `getNumSamples()` returns the current sub-range size, not the original file length. |
| `af.loadFile("C:/audio/file.wav")` | `af.loadFile("{PROJECT_FOLDER}file.wav")` | `loadFile` expects a HISE pool reference string, not an absolute file path. |
| `af.linkTo(table)` where table is a Table | `af.linkTo(otherAudioFile)` | `linkTo` requires the same data type -- linking AudioFile to Table throws a type mismatch error. |

## codeExample
```javascript
// Get an AudioFile handle from an AudioLoopPlayer
const var asp = Synth.getAudioSampleProcessor("AudioLoopPlayer1");
const var af = asp.getAudioFile(0);

// Load a file and set up a content callback
af.loadFile("{PROJECT_FOLDER}audio/loop.wav");

af.setContentCallback(function(notification)
{
    Console.print("Audio content changed, samples: " + af.getNumSamples());
});
```

## Alternatives
- **Table** -- Use for editable modulation curves; AudioFile is for audio waveform data with file loading.
- **SliderPackData** -- Use for discrete slider value arrays; AudioFile is for audio sample data with file I/O.
- **DisplayBuffer** -- Use for ring buffer visualization; AudioFile is for loading and manipulating audio files.
- **Buffer** -- Use for standalone in-memory float arrays; AudioFile is for file-backed audio slots with sample rate and range management.
- **ScriptAudioWaveform** -- The UI component counterpart; AudioFile is the data handle for programmatic control.

## Related Preprocessors
None.

## Diagrams

### complex-audio-data-chain
- **Brief:** Audio File Data Chain
- **Type:** topology
- **Description:** Audio file workflows use a three-part chain. `AudioSampleProcessor` selects the processor that owns one or more audio file slots, `AudioFile` exposes the complex data stored in a specific slot, and `ScriptAudioWaveform` displays or edits that same slot in the UI. The binding pair is `processorId` plus `sampleIndex`, which is a complex-data connection and not the normal `parameterId` parameter binding path.

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: The two callback methods already have ADD_CALLBACK_DIAGNOSTIC registrations in C++. No additional parse-time diagnostics are warranted -- the class has no timeline dependencies, no silent-failure preconditions beyond standard null checks, and no mode-dependent API surfaces.
