# ScriptAudioWaveform -- Class Analysis

## Brief
Audio waveform display component for visualizing AudioFile data with range selection and sampler integration.

## Purpose
ScriptAudioWaveform is a UI component that displays audio waveform data and provides interactive range selection. It operates in two modes: AudioFile mode (connected to any processor with AudioFile external data) where it shows a multi-channel audio buffer with file browsing and drag-and-drop support, or Sampler mode (connected to a ModulatorSampler) where it shows the currently playing sample with play/loop/crossfade areas. The component inherits from ComplexDataScriptComponent, which manages the connection between the UI display and the underlying MultiChannelAudioBuffer data object through HISE's ExternalData system.

## Details

### Dual Rendering Mode

The component automatically selects its rendering mode based on the connected processor type:

| Mode | Connected Processor | JUCE Component | Features |
|------|-------------------|----------------|----------|
| **AudioFile** | Any processor with AudioFile data (or none) | `MultiChannelAudioBufferDisplay` | File browser, drag-and-drop, filename overlay, range selection |
| **Sampler** | `ModulatorSampler` | `SamplerSoundWaveform` | Auto-tracks playing voice, play/loop/crossfade areas, sample property editing |

The mode is determined at wrapper creation time from the `processorId` property. Both modes share the `AudioDisplayComponent` base which provides the `HiseAudioThumbnail` renderer and `SampleArea` system.

### Sampler Mode -- sampleIndex Behavior

In Sampler mode, the `sampleIndex` property controls which sound is displayed:
- **`-1` (or `0` default with auto-tracking):** The `SamplerListener` is active and automatically displays the most recently started voice's sample. This updates in real-time as notes play.
- **`>= 0`:** The listener is deactivated and a specific sound is displayed by its index in the sampler's sound list.

### ComplexDataScriptComponent Data Source Resolution

The component's audio data comes from (in priority order):
1. **External reference** (set via `referToData`) -- a ScriptAudioFile or another ScriptAudioWaveform. See `referToData()` for details.
2. **Connected processor** (set via `processorId` property) -- the processor's AudioFile at the given `sampleIndex`. See `registerAtParent()` for the reverse direction.
3. **Internal owned object** -- a default MultiChannelAudioBuffer created at construction

### Colour Mapping

| Script Property | Waveform Element |
|----------------|-----------------|
| `bgColour` | Component background |
| `itemColour` | Waveform outline |
| `itemColour2` | Waveform fill |
| `textColour` | Text overlay (filename) |
| `itemColour3` | Additional colour (semi-transparent white by default) |

### Deactivated Properties

These inherited properties are deactivated (not shown in property editor):
`text`, `min`, `max`, `defaultValue`, `macroControl`, `parameterId`, `linkedTo`, `isMetaParameter`, `isPluginParameter`, `pluginParameterName`, `automationId`

## obtainedVia
`Content.addAudioWaveform(name, x, y)`

## minimalObjectToken
wf

## Constants
(None)

## Dynamic Constants
(None)

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| `wf.setDefaultFolder("/path/to/folder");` | `wf.setDefaultFolder(FileSystem.getFolder(FileSystem.Documents));` | `setDefaultFolder` requires a `File` object, not a string path. Passing a string causes a script error. |

## codeExample
```javascript
const var wf = Content.addAudioWaveform("Waveform1", 0, 0);
wf.set("width", 400);
wf.set("height", 150);
```

## Alternatives
- **ScriptTable** -- Use for envelope/curve editing instead of audio visualization.
- **ScriptPanel** -- Use for fully custom-drawn waveform visualizations with complete control over rendering.
- **AudioFile** -- The data handle counterpart; ScriptAudioWaveform is the UI component that displays AudioFile content.

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
Rationale: ScriptAudioWaveform methods are straightforward data queries and display setters with no timeline dependencies or silent-failure preconditions that would benefit from parse-time diagnostics.
