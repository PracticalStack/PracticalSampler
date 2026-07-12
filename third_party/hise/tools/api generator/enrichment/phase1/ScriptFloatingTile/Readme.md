# ScriptFloatingTile -- Class Analysis

## Brief
Embeds pre-built HISE floating tile widgets (keyboard, preset browser, meters, etc.) into the script interface.

## Purpose
ScriptFloatingTile is a ScriptComponent subclass that wraps the HISE FloatingTile system for use on the script interface. It embeds pre-built UI panels -- preset browser, MIDI keyboard, performance meters, envelope graphs, filter displays, and more -- by specifying a content type and optional JSON configuration. Unlike ScriptPanel which requires custom drawing code, ScriptFloatingTile provides access to complex built-in widgets with minimal scripting. The component manages an internal JSON data object that configures the embedded panel's type, colours, font, and panel-specific properties.

## Details

### Property Architecture

ScriptFloatingTile adds 6 properties beyond the ScriptComponent base and aggressively deactivates 17 inherited properties that are irrelevant for display-only widgets (saveInPreset, macroControl, min/max, processorId, text, tooltip, etc.).

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `itemColour3` | Colour | `0` (transparent) | Fifth colour slot for floating tile content |
| `updateAfterInit` | Toggle | `true` | Whether setValue() triggers a full content reload |
| `ContentType` | Choice | `"Empty"` | Panel type identifier string |
| `Font` | Choice | `"Default"` | Font name for text-rendering panels |
| `FontSize` | Number | `14.0` | Font size for text-rendering panels |
| `Data` | Code | `"{\n}"` | JSON string for panel-specific configuration |

The `saveInPreset` property defaults to `false` (unlike most components), reflecting that floating tiles are display elements, not user controls.

### JSON Configuration System

The component maintains an internal JSON data object that is passed to the underlying FloatingTile. This object has a standard structure:

```javascript
{
    "Type": "PresetBrowser",
    "Font": "Oxygen Bold",
    "FontSize": 16.0,
    "ColourData": {
        "bgColour": "0xFF222222",
        "textColour": "0xFFFFFFFF",
        "itemColour1": "0xFF444444",
        "itemColour2": "0xFF666666",
        "itemColour3": "0xFF888888"
    }
    // ... panel-specific properties
}
```

When ScriptComponent colour properties are changed, they are automatically mapped into the `ColourData` sub-object. Note: the base `itemColour` property maps to `"itemColour1"` in ColourData.

See `setContentData()` for the full JSON configuration API and property details.

### Content Type Resolution

Only frontend-available panel types can be embedded. The available types are determined by `FloatingTileContent::Factory::registerFrontendPanelTypes()`:

| ContentType | Widget |
|-------------|--------|
| `"Empty"` | Empty placeholder |
| `"PresetBrowser"` | Preset browser (bank/category/preset columns) |
| `"AboutPagePanel"` | About page with version info |
| `"Keyboard"` | Virtual MIDI keyboard |
| `"PerformanceLabel"` | CPU/voice count statistics |
| `"MidiOverlayPanel"` | MIDI player transport overlay |
| `"ActivityLed"` | MIDI activity indicator |
| `"CustomSettings"` | Audio/MIDI device settings |
| `"MidiSources"` | MIDI input source selector |
| `"MidiChannelList"` | MIDI channel filter |
| `"TooltipPanel"` | Tooltip display area |
| `"MidiLearnPanel"` | MIDI learn assignment UI |
| `"FrontendMacroPanel"` | Macro control knobs |
| `"Plotter"` | Signal plotter |
| `"AudioAnalyser"` | FFT/oscilloscope/goniometer |
| `"Waveform"` | Wavetable preview |
| `"FilterDisplay"` | Filter frequency response |
| `"DraggableFilterPanel"` | Interactive filter control |
| `"WavetableWaterfall"` | Waterfall/spectrogram display |
| `"MPEPanel"` | MPE configuration |
| `"ModulationMatrix"` | Modulation matrix editor |
| `"ModulationMatrixController"` | Modulation matrix controls |
| `"AHDSRGraph"` | AHDSR envelope graph |
| `"FlexAHDSRGraph"` | Flex AHDSR envelope graph |
| `"MarkdownPanel"` | Markdown text renderer |
| `"MatrixPeakMeter"` | Multi-channel peak meter |

Backend-only panels (ScriptEditor, Console, SampleEditor, etc.) are not available.

### Content Reload Behavior

All visual property changes (colours, font, font size, data, content type) trigger a complete content reload -- the entire floating tile panel is destroyed and recreated. This is because the FloatingTile system uses monolithic JSON configuration.

The `updateAfterInit` property (default: `true`) controls whether `setValue()` triggers a content reload. See `setValue()` for details on the reload mechanism and performance considerations.

### LookAndFeel Propagation

See `setLocalLookAndFeel()` for details on recursive LAF propagation and CSS initialization behavior.

## obtainedVia
`Content.addFloatingTile(name, x, y)`

## minimalObjectToken
ft

## Constants
None. ScriptFloatingTile has no `addConstant()` calls.

## Dynamic Constants
None.

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| `ft.setContentData({"Type": "Console"})` | `ft.setContentData({"Type": "PresetBrowser"})` | Only frontend panel types are available. Backend-only types like "Console" or "ScriptEditor" are not registered and will produce an empty tile. |

## codeExample
```javascript
const var ft = Content.addFloatingTile("FloatingTile1", 0, 0);
ft.setContentData({"Type": "PresetBrowser"});
ft.set("width", 600);
ft.set("height", 400);
```

## Alternatives
- `ScriptPanel` -- Use for fully custom-drawn components with paint routines and mouse callbacks. ScriptFloatingTile embeds built-in widgets without custom code.
- `ScriptAudioWaveform` -- Dedicated component for audio waveform display. ScriptFloatingTile can embed broader built-in widgets like preset browsers and keyboards.

## Related Preprocessors
- `USE_BACKEND` -- Enables automatic property sync when ContentType changes (reads panel default colours back into ScriptComponent properties)
- `HI_ENABLE_EXTERNAL_CUSTOM_TILES` -- Allows third-party panel types to be registered and available as ContentType options

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: ScriptFloatingTile has minimal scripting surface (one unique method). Invalid ContentType values silently produce an empty tile, but this is the normal FloatingTile system behavior and would require runtime panel type validation that doesn't exist at parse time.
