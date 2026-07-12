<!-- Diagram triage:
  - No diagrams specified in Phase 1 data
-->

# ScriptAudioWaveform

ScriptAudioWaveform is a UI component that displays audio waveform data with interactive range selection and playback cursor support. It operates in one of two modes depending on the connected processor:

| Mode | When active | Features |
|------|------------|----------|
| AudioFile | Connected to any processor with AudioFile data (or no connection) | File browser, drag-and-drop loading, filename overlay, range selection |
| Sampler | Connected to a sampler module | Auto-tracks the playing voice, displays play/loop/crossfade areas |

The mode is determined automatically from the `processorId` property. In Sampler mode, the `sampleIndex` property controls which sound is displayed: leave it at the default to auto-track the most recently started voice, or set it to a specific index to pin the display to one sound.

```js
const var wf = Content.addAudioWaveform("Waveform1", 0, 0);
wf.set("width", 400);
wf.set("height", 150);
```

The component resolves its audio data source in priority order:

1. An external reference set via `referToData()` - a ScriptAudioFile or another ScriptAudioWaveform
2. A connected processor set via the `processorId` property
3. An internal buffer created at construction

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

Colour properties control the waveform appearance:

| Property | Element |
|----------|---------|
| `bgColour` | Component background |
| `itemColour` | Waveform outline |
| `itemColour2` | Waveform fill |
| `textColour` | Text overlay (filename) |
| `itemColour3` | Additional colour |

For fully custom rendering, attach a look-and-feel object with `setLocalLookAndFeel()` and register the thumbnail draw functions (`drawThumbnailBackground`, `drawThumbnailPath`, `drawThumbnailRuler`, `drawThumbnailRange`, `drawThumbnailText`) and `getThumbnailRenderOptions`.

> [!Tip:Remaining methods common to all components] The remaining methods on this component are common to all UI components.

## Common Mistakes

- **Pass File object to setDefaultFolder**
  **Wrong:** `wf.setDefaultFolder("/path/to/folder");`
  **Right:** `wf.setDefaultFolder(FileSystem.getFolder(FileSystem.Documents));`
  *`setDefaultFolder` requires a File object, not a string path. Passing a string causes a script error.*

- **Reset cursor after changing processorId**
  **Wrong:** Calling `set("processorId", newId)` without resetting the playback cursor
  **Right:** `set("processorId", newId)` followed by `setPlaybackPosition(0)`
  *When rebinding to a different processor, the playback cursor retains its position from the previous audio file. Reset it to avoid a stale cursor position.*

- **Use setContentCallback not timer polling**
  **Wrong:** Polling for audio file changes with a Timer
  **Right:** Using `Broadcaster.attachToComplexData("AudioFile.Content", ...)` with `addComponentRefreshListener`
  *Broadcasters provide event-driven repaint triggers without the overhead and latency of timer-based polling.*

- **Set all colour properties together**
  **Wrong:** Setting only `itemColour` when changing the waveform colour
  **Right:** Setting both `itemColour` (outline) and `itemColour2` (fill)
  *The waveform uses two colour properties for its appearance. Changing only one produces a mismatched look.*
