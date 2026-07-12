---
title: "Audio Waveform"
description: "Audio waveform display with interactive playback position and range selection, connected to AudioSampleProcessor modules."
componentId: "ScriptAudioWaveform"
componentType: "plugin-component"
screenshot: "/images/v2/reference/ui-components/audio-waveform.png"
llmRef: |
  ScriptAudioWaveform (UI component)
  Create via: Content.addAudioWaveform("name", x, y)
  Scripting API: $API.ScriptAudioWaveform$

  Audio waveform display for visualising AudioFile data with interactive range selection. Operates in AudioFile mode (file browsing, drag-and-drop, range selection) or Sampler mode (auto-tracks playing voice, shows play/loop/crossfade areas) depending on connected processor type. Connects via processorId to an AudioSampleProcessor.

  Properties (component-specific):
    itemColour3: additional accent colour
    opaque: skip parent repaint when true
    showLines: show horizontal guide lines
    showFileName: show filename text overlay
    sampleIndex: audio file slot index
    enableRange: enable range selection
    loadWithLeftClick: load file on left click

  Customisation:
    LAF: drawThumbnailBackground, drawThumbnailText, drawThumbnailPath, drawThumbnailRange, drawThumbnailRuler, getThumbnailRenderOptions (config callback)
    CSS: .scriptaudiowaveform with :disabled; .playhead, .waveformedge sub-selectors; --waveformPath, --playhead variables
    Filmstrip: no
seeAlso: []
commonMistakes:
  - title: "Passing a string path to setDefaultFolder"
    wrong: "wf.setDefaultFolder(\"/path/to/folder\")"
    right: "wf.setDefaultFolder(FileSystem.getFolder(FileSystem.Desktop))"
    explanation: "setDefaultFolder() requires a File object, not a string path. Use FileSystem.getFolder() to obtain a File object."
  - title: "Switching processorId without resetting the cursor"
    wrong: "wf.set(\"processorId\", newId) — cursor stays at previous file position"
    right: "wf.set(\"processorId\", newId); wf.setPlaybackPosition(0);"
    explanation: "When switching the connected processor, the playback cursor retains the position from the previous audio file. Reset it with setPlaybackPosition(0) after changing processorId."
  - title: "Setting only itemColour when changing waveform colour"
    wrong: "Setting itemColour alone for a new waveform colour"
    right: "Set both itemColour (outline) and itemColour2 (fill) for a consistent appearance"
    explanation: "itemColour controls the outline and itemColour2 controls the fill. Setting only one creates a mismatched look."
  - title: "Polling for audio file changes with a Timer"
    wrong: "Using a Timer callback to check if the audio file has changed"
    right: "Use Broadcaster.attachToComplexData(\"AudioFile.Content\", ...) for change notifications"
    explanation: "Timer polling adds unnecessary overhead and latency. The Broadcaster system provides immediate notifications when audio file content changes."
  - title: "Expecting processorId switch to refresh the display"
    wrong: "wf.set(\"processorId\", \"Sampler2\"); wf.changed(); — display still shows previous waveform"
    right: "Use referToData() to switch the data source at runtime, or rebind via the AudioSampleProcessor API"
    explanation: "Setting processorId via script updates the internal property but may not refresh the waveform display until recompilation. For runtime switching between processors, use referToData() with the target processor's audio file handle instead."
---

![Audio Waveform](/images/v2/reference/ui-components/audio-waveform.png)

ScriptAudioWaveform displays an audio file waveform with interactive playback position and range selection. It connects to modules that implement the AudioSampleProcessor interface — such as the Audio Loop Player or Convolution Reverb — via the `processorId` property.

The component operates in two modes depending on the connected processor type. In **AudioFile mode**, it supports file browsing, drag-and-drop loading, and interactive range selection for defining loop points or sample regions. In **Sampler mode**, it automatically tracks the currently playing voice and displays play position, loop regions, and crossfade areas.

The data source is resolved in priority order: (1) an external reference set with `referToData()`, (2) a connected processor selected by `processorId`, (3) an internal buffer created by the component itself. Use `sampleIndex` to select which audio slot to display when a processor has multiple slots.

## Properties

Set properties with `ScriptAudioWaveform.set(property, value)`.

### Component-specific properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| *`itemColour3`* | hex String | `"0x22FFFFFF"` | Additional accent colour for waveform rendering. |
| *`opaque`* | bool | `true` | When `true`, skips parent component repaint for better performance. Set to `false` and `bgColour` to transparent when layering over a custom-painted ScriptPanel background. |
| *`showLines`* | bool | `false` | Show horizontal guide lines across the waveform display. |
| *`showFileName`* | bool | `true` | Show a text overlay with the loaded filename. Set to `false` when using a custom LAF that handles text display via `drawThumbnailText`. |
| *`sampleIndex`* | int | `0` | Selects which audio file slot on the connected processor to display. Leave at default for auto-tracking in Sampler mode, or set a specific index to pin the display. |
| *`enableRange`* | bool | `true` | Enable interactive range selection (draggable edges for defining sample regions). Set to `false` for display-only waveforms. |
| *`loadWithLeftClick`* | bool | `false` | When `true`, left-clicking the waveform opens a file browser to load a new audio file. |

> [!Warning:Both opaque and bgColour must be set for transparent layering] Setting `bgColour` to transparent without also setting `opaque` to `false` causes rendering artifacts — the parent component's graphics bleed through as visual noise. Always set both: `opaque = false` and `bgColour = 0x00000000`.

### Common properties

| Property | Description |
|----------|-------------|
| `x`, `y`, `width`, `height` | Position and size in pixels, relative to parent |
| `tooltip` | Hover tooltip |
| `visible`, `enabled`, `locked` | Display and interaction state |
| `bgColour`, `itemColour`, `itemColour2`, `textColour` | Colour properties |
| `parentComponent` | Parent component for layout nesting |
| `saveInPreset`, `useUndoManager`, `deferControlCallback` | Preset persistence, undo, and callback deferral |
| `processorId` | Connected AudioSampleProcessor module ID |

### Deactivated properties

The following properties are deactivated for ScriptAudioWaveform:

`macroControl`, `parameterId`, `linkedTo`, `isMetaParameter`, `isPluginParameter`, `pluginParameterName`, `automationID`, `text`, `min`, `max`, `defaultValue`.

## LAF Customisation

Register a custom look and feel to control the rendering of the waveform display. Six functions cover all visual elements, including a special non-drawing configuration callback.

### LAF Functions

| Function | Description |
|----------|-------------|
| `drawThumbnailBackground` | Draws the background of the waveform display |
| `drawThumbnailText` | Draws the filename text overlay |
| `drawThumbnailPath` | Draws the waveform path |
| `drawThumbnailRange` | Draws a range selection overlay |
| `drawThumbnailRuler` | Draws the playback position ruler |
| `getThumbnailRenderOptions` | Non-drawing callback that returns render configuration (see separate table) |

### `obj` Properties — `drawThumbnail*` functions (shared)

These properties are shared across `drawThumbnailBackground`, `drawThumbnailPath`, `drawThumbnailRange`, and `drawThumbnailRuler`:

| Property | Type | Description |
|----------|------|-------------|
| `obj.id` | String | The component's ID |
| `obj.bgColour` | int (ARGB) | Background colour |
| `obj.itemColour` | int (ARGB) | Fill colour |
| `obj.textColour` | int (ARGB) | Outline colour |

### Additional `obj` properties per function

#### `drawThumbnailBackground`

| Property | Type | Description |
|----------|------|-------------|
| `obj.area` | Array[x,y,w,h] | The thumbnail area |
| `obj.enabled` | bool | Whether the area is enabled |

#### `drawThumbnailText`

| Property | Type | Description |
|----------|------|-------------|
| `obj.area` | Array[x,y,w,h] | The text area |
| `obj.text` | String | The filename text to display |

#### `drawThumbnailPath`

| Property | Type | Description |
|----------|------|-------------|
| `obj.area` | Array[x,y,w,h] | The path bounds |
| `obj.enabled` | bool | Whether the area is enabled (false for inactive zones) |
| `obj.path` | Path | The waveform path object |

#### `drawThumbnailRange`

| Property | Type | Description |
|----------|------|-------------|
| `obj.area` | Array[x,y,w,h] | The range area |
| `obj.rangeIndex` | int | Index of the range |
| `obj.rangeColour` | int (ARGB) | The range highlight colour |
| `obj.enabled` | bool | Whether the range is enabled |

#### `drawThumbnailRuler`

| Property | Type | Description |
|----------|------|-------------|
| `obj.area` | Array[x,y,w,h] | The thumbnail area |
| `obj.xPosition` | int | The X pixel position of the playback ruler |

### `getThumbnailRenderOptions` — Configuration callback

This is a non-drawing callback that returns a configuration object controlling how the waveform is rendered internally. Unlike other LAF functions, it does not receive a Graphics object — it receives only `obj` and should return the modified object.

| Property | Type | Description |
|----------|------|-------------|
| `obj.displayMode` | int | Display mode enum value |
| `obj.manualDownSampleFactor` | double | Manual downsample factor for performance tuning |
| `obj.drawHorizontalLines` | bool | Whether to draw horizontal guide lines |
| `obj.scaleVertically` | bool | Scale waveform to fill vertical space (useful for IR displays) |
| `obj.displayGain` | double | Display gain multiplier |
| `obj.useRectList` | bool | Use rectangle list rendering mode |
| `obj.forceSymmetry` | bool | Force symmetric waveform display (useful for one-shot drum samples) |
| `obj.multithreadThreshold` | int | Sample count threshold for multithreaded rendering |
| `obj.dynamicOptions` | bool | Whether options update dynamically |

### Example

```javascript
const var wf = Content.addAudioWaveform("Waveform1", 10, 10);
wf.set("width", 400);
wf.set("height", 150);

const var laf = Content.createLocalLookAndFeel();

laf.registerFunction("drawThumbnailBackground", function(g, obj)
{
    g.setColour(obj.bgColour);
    g.fillRoundedRectangle(obj.area, 5.0);
});

laf.registerFunction("drawThumbnailPath", function(g, obj)
{
    if (obj.enabled)
    {
        // Active zone — fill and outline
        g.setColour(Colours.withAlpha(obj.itemColour, 0.5));
        g.fillPath(obj.path, obj.area);
        g.setColour(obj.itemColour);
        g.drawPath(obj.path, obj.area, 1.0);
    }
    else
    {
        // Inactive zone — dimmed
        g.setColour(Colours.withAlpha(obj.itemColour, 0.15));
        g.fillPath(obj.path, obj.area);
    }
});

laf.registerFunction("drawThumbnailText", function(g, obj)
{
    g.setColour(Colours.withAlpha(Colours.white, 0.7));
    g.setFont("Arial", 12.0);
    g.drawAlignedText(obj.text, [obj.area[0] + 5, obj.area[1] + 5,
                                  obj.area[2] - 10, 20], "left");
});

laf.registerFunction("drawThumbnailRange", function(g, obj)
{
    g.setColour(Colours.withAlpha(obj.rangeColour, 0.3));
    g.fillRect(obj.area);
});

laf.registerFunction("drawThumbnailRuler", function(g, obj)
{
    g.setColour(Colours.white);
    g.drawLine(obj.xPosition, obj.xPosition,
               obj.area[1], obj.area[1] + obj.area[3], 2.0);
});

// Configure render options for clean display
laf.registerFunction("getThumbnailRenderOptions", function(obj)
{
    obj.forceSymmetry = true;
    obj.scaleVertically = true;
    return obj;
});

wf.setLocalLookAndFeel(laf);
```

> [!Tip:Use Broadcaster for file load and range change notifications] The ScriptAudioWaveform control callback does not fire when users load files or drag range edges. Use `Broadcaster.attachToComplexData("AudioFile.Content", processorId, sampleIndex)` to receive notifications when the audio file content changes, and monitor range changes via the AudioSampleProcessor's `setFile` API or a periodic check on `getRangeStart()` / `getRangeEnd()`.

## CSS Styling

CSS provides control over the waveform background, path, playhead, range edges, and filename label using multiple selectors. The waveform path is rendered via a CSS variable, consistent with the Table component's path rendering approach.

### Selectors

| Selector | Type | Description |
|----------|------|-------------|
| `.scriptaudiowaveform` | Class | Default class selector for the waveform component |
| `#AudioWaveform1` | ID | Targets a specific waveform by component name |

### Pseudo-states

| State | Description |
|-------|-------------|
| `:disabled` | Inactive zones (outside the sample range) |

### Pseudo-elements

| Element | Description |
|---------|-------------|
| `::before` | Waveform path rendering via `background-image: var(--waveformPath)` |
| `::after` | Additional overlay |

### CSS Variables

| Variable | Description |
|----------|-------------|
| `--waveformPath` | Base64-encoded waveform path — use as `background-image` |
| `--playhead` | Normalised playback position (0.0–1.0) |
| `--bgColour` | Background colour from component properties |
| `--itemColour` | From the `itemColour` property |
| `--itemColour2` | From the `itemColour2` property |
| `--textColour` | Text colour from component properties |

### Sub-selectors

| Selector | Description |
|----------|-------------|
| `.playhead` | Playback position indicator (use `::before`/`::after` with `var(--playhead)`) |
| `.waveformedge` | Draggable range edges |
| `.waveformedge:first-child` | Left range edge |
| `.waveformedge:last-child` | Right range edge |
| `label` | Filename text overlay |

### Edge pseudo-states

| State | Description |
|-------|-------------|
| `.waveformedge:hover` | Mouse over a range edge |
| `.waveformedge:active` | Dragging a range edge |

### Example Stylesheet

```javascript
const var wf = Content.addAudioWaveform("AudioWaveform1", 10, 10);
wf.set("width", 400);
wf.set("height", 150);

const var laf = Content.createLocalLookAndFeel();

laf.setInlineStyleSheet("
/** Draw the background. */
.scriptaudiowaveform
{
	background: #929;
	box-shadow: inset 0px 4px 3px rgba(0, 0, 0, 0.3);
	border-radius: 5px;
}

/** Inactive zones. */
.scriptaudiowaveform:disabled
{
	background: yellow;
}

/** Draw the waveform path. */
.scriptaudiowaveform::before
{
	content: '';
	background-image: var(--waveformPath);
	background-color: red;
	border: 1px solid white;
	box-shadow: 0px 1px 2px black;
}

/** Inactive waveform path. */
.scriptaudiowaveform::before:disabled
{
	content: '';
	background-image: var(--waveformPath);
	background-color: orange;
	border: 0px solid white;
	box-shadow: none;
}

/** Range edges. */
.waveformedge
{
	border-color: rgba(255,255,255, 0.3);
}

.waveformedge:hover
{
	background: rgba(255, 255, 255, 0.5);
	border-color: white;
}

.waveformedge:first-child { border-left: 1px; }
.waveformedge:last-child { border-right: 1px; }

/** Filename label. */
label
{
	color: white;
	background-color: color-mix(in rgb, orange 20%, black);
	margin: 2px;
	padding: 2px 5px;
	font-size: 0.9em;
	text-align: right;
	vertical-align: top;
	border-radius: 3px;
}

/** Playback position indicator. */
.playhead::before
{
	content: '';
	background: rgba(0, 0, 0, 0.5);
	width: 10px;
	left: calc(calc(100% * var(--playhead)) - 4px);
}

.playhead::after
{
	content: '';
	background: red;
	width: 2px;
	left: calc(100% * var(--playhead));
}
");

wf.setLocalLookAndFeel(laf);
```

> [!Warning:setPlaybackPosition may bounce back in Sampler mode] When connected to a Sampler, calling `setPlaybackPosition()` sets the cursor momentarily but it snaps back to the voice's actual playback position on the next repaint. This function works reliably in AudioFile mode (Audio Loop Player, Convolution Reverb) but not when the Sampler continuously overrides the position from the active voice.

## Notes

- **Two operating modes.** In AudioFile mode (connected to Audio Loop Player, Convolution Reverb, etc.), the waveform supports file browsing and range selection. In Sampler mode, it auto-tracks the playing voice and displays play/loop/crossfade areas. The mode is determined automatically by the connected processor type.
- **Data source priority:** `referToData()` overrides `processorId`, which overrides the internal buffer. Use `referToData()` for runtime data switching without changing the processor connection.

> [!Tip:Use referToData() instead of processorId for runtime switching] Changing `processorId` at runtime via `set()` may not refresh the waveform display. Instead, get the target processor's audio file handle with `Synth.getAudioSampleProcessor(id).getAudioFile(0)` and pass it to `referToData()`. This reliably updates both the data binding and the visual display without requiring recompilation.

- **Transparent layering.** Set `opaque` to `false` and `bgColour` to `0x00000000` (transparent) when layering the waveform over a custom-painted ScriptPanel background.
- **`getThumbnailRenderOptions`** is unique among LAF functions — it receives only `obj` (no Graphics parameter) and must return the modified object. Use it to configure rendering without custom drawing: set `forceSymmetry` to `true` for one-shot drum samples, `scaleVertically` to `true` for IR displays.
- **Reset cursor after processor switch.** When changing `processorId` at runtime, follow with `setPlaybackPosition(0)` to reset the stale cursor from the previous audio file.
- **Use `getRangeEnd()`** to get the total sample count of the loaded audio for coordinate calculations (e.g., overlaying envelope shapes). Returns 0 when no audio is loaded.
- **Playhead rendering** uses the same `calc(100% * var(--playhead))` pattern in CSS as the Table component's `--playhead` variable, ensuring consistent behaviour across components.
- **Complex data binding** uses `processorId` + `sampleIndex`, not the normal `processorId` + `parameterId` path. The `parameterId` property is deactivated for this component.

**See also:** <!-- populated during cross-reference post-processing -->
