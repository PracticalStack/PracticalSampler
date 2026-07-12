<!-- Diagram triage:
  - No diagrams specified in Phase 1 data (class-level diagrams[] is empty, no method-level diagram fields)
-->

# AudioFile

AudioFile is a data handle for loading and manipulating audio sample data in a processor's audio file slot. It is one of the five complex data types in HISE, alongside Table, SliderPackData, DisplayBuffer, and Buffer.

Obtain an AudioFile handle in one of three ways:

1. `AudioSampleProcessor.getAudioFile(slotIndex)` - returns a reference to an existing processor's audio slot (AudioLooper, ConvolutionEffect, WavetableSynth)
2. `Engine.createAndRegisterAudioFile(index)` - creates a standalone data slot not tied to a specific processor
3. `ScriptAudioWaveform.registerAtParent()` - registers a UI waveform display's content at the script processor and returns the AudioFile reference

```js
const var asp = Synth.getAudioSampleProcessor("AudioLoopPlayer1");
const var af = asp.getAudioFile(0);
```

The loaded audio data operates on two layers: the original file buffer and an optional sub-range. After calling `setRange()`, methods like `getContent()` and `getNumSamples()` return data for the sub-range only. Use `getTotalLengthInSamples()` to query the original file length.

Two callback types are available for monitoring changes:

- Content callbacks fire when a file is loaded, the buffer is modified, or the range changes
- Display callbacks fire when the playback position updates during audio playback

For monitoring multiple audio file slots simultaneously, use `Broadcaster.attachToComplexData("AudioFile.Content", ...)` instead of registering individual content callbacks. A single broadcaster can watch all processor slots at once, with the processor ID passed as a callback argument for routing. For live playback position tracking, `Broadcaster.attachToComplexData()` also supports `AudioFile.DisplayIndex` mode.

## Complex Data Chain

Audio file workflows use a three-part complex-data chain:

![Audio File Data Chain](topology_complex-audio-data-chain.svg)

- `AudioSampleProcessor` selects the module that owns one or more audio file slots.
- `AudioFile` is the complex-data handle for one slot within that module.
- `ScriptAudioWaveform` displays or edits one selected slot in the UI.

Use the binding properties separately:

- `processorId` selects the owning processor.
- `sampleIndex` selects which audio slot inside that processor should be displayed.

This is not the normal parameter binding path. `parameterId` targets processor parameters, while audio-slot binding uses `sampleIndex` instead.

> [!Tip:Handles reference data slots, not files] AudioFile handles are references to data slots, not file objects. Loading a file with `loadFile()` populates the slot with audio data from the pool. To work with actual files on disk, pass the reference string from `getCurrentlyLoadedFile()` to `FileSystem.fromReferenceString()` to obtain a File object.

## Common Mistakes

- **Use getTotalLengthInSamples for full file length**
  **Wrong:** `af.getNumSamples()` after `setRange()` expecting the full file length
  **Right:** `af.getTotalLengthInSamples()`
  *`getNumSamples()` returns the current sub-range size after `setRange()`, not the original file length.*

- **Use pool references not disk paths**
  **Wrong:** `af.loadFile("C:/audio/file.wav")`
  **Right:** `af.loadFile("{PROJECT_FOLDER}file.wav")`
  *`loadFile()` expects a HISE pool reference string. Use `Engine.loadAudioFilesIntoPool()` to get valid references, or `file.toString(0)` when loading from a file browser result.*

- **Link only to same complex data type**
  **Wrong:** `af.linkTo(table)` where `table` is a Table reference
  **Right:** `af.linkTo(otherAudioFile)`
  *`linkTo()` requires the same complex data type. Linking an AudioFile to a Table or SliderPackData produces a type mismatch error.*

- **Use Broadcaster to monitor multiple slots**
  **Wrong:** Registering `setContentCallback()` on each of 12+ AudioFile handles individually
  **Right:** Using one `Broadcaster.attachToComplexData("AudioFile.Content", arrayOfModuleIds, 0, ...)`
  *A single broadcaster can monitor all processor slots at once, with the processor ID passed as a callback argument. Individual callbacks create redundant boilerplate and miss the broadcaster's built-in features like delayed listeners and queuing.*
