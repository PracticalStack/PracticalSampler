---
title: Floating Tiles
description: Pre-built UI panels for common plugin interface features - preset browsing, visualisation, MIDI, and modulation
---

Floating tiles are ready-made UI panels for standard plugin features. Instead of scripting a preset browser or audio analyser from scratch, you drop in a floating tile, set its ContentType, and configure it via JSON properties.

## ScriptFloatingTile as Host

A floating tile is embedded in the interface through a [ScriptFloatingTile]($API.ScriptFloatingTile$) component - a plugin component that acts as a host container for the actual panel. You create it like any other component:

```javascript
const var ft = Content.addFloatingTile("Browser", 0, 0);
ft.setContentData({"ContentType": "PresetBrowser",
                    "ShowFavoriteIcon": true,
                    "NumColumns": 3});
```

The `ScriptFloatingTile` is itself a plugin component with all the standard properties (`x`, `y`, `width`, `height`, `visible`, `parentComponent`, etc.). The floating tile panel rendered inside it is determined by the `ContentType` property.

## ContentType System

Each floating tile type is identified by a `ContentType` string. The JSON properties object configures the tile's behaviour and appearance. Common properties shared by most tiles:

- `Font` - font family for text rendering
- `FontSize` - font size in pixels
- `ProcessorId` - for tiles that connect to an audio module, the module's ID string

Type-specific properties are documented on each tile's individual page.

## Styling

Floating tiles support the same two styling systems as plugin components:

**Look and Feel (LAF)** - each tile type exposes named LAF functions for custom drawing. For example, the PresetBrowser exposes `drawPresetBrowserBackground`, `drawPresetBrowserListItem`, and several others.

**CSS** - tiles can be styled via CSS selectors that target internal sub-components. The CSS approach allows deeper customisation of complex tiles like the PresetBrowser or Keyboard.

Both are documented on each tile's page.

## Floating Tiles

### Preset and Settings

| Floating Tile | Related | Description |
|--------------|---------|-------------|
| [PresetBrowser]($UI.PresetBrowser$) | $API.UserPresetHandler$ | Multi-column preset browser with search, favourites, tags, and notes |
| [CustomSettings]($UI.CustomSettings$) | $API.Engine$ | Audio driver, buffer size, zoom factor, and streaming mode settings |
| [AboutPagePanel]($UI.AboutPagePanel$) | - | Product info, version, copyright, and licensing display |

---

### Visualisation

| Floating Tile | Related | Description |
|--------------|---------|-------------|
| [AHDSRGraph]($UI.AHDSRGraph$) | $MODULES.AHDSR$ | Envelope shape visualiser for AHDSR modulators |
| [AudioAnalyser]($UI.AudioAnalyser$) | $MODULES.Analyser$ | Goniometer, oscilloscope, and spectrum analyser |
| [FilterDisplay]($UI.FilterDisplay$) | $MODULES.PolyphonicFilter$ | Static frequency response curve |
| [DraggableFilterPanel]($UI.DraggableFilterPanel$) | $MODULES.CurveEq$ | Interactive draggable EQ node display |
| [Plotter]($UI.Plotter$) | - | Oscilloscope for modulation signals |
| [Waveform]($UI.Waveform$) | $MODULES.WaveSynth$ | Oscillator waveform display for sine and waveform generators |
| [WavetableWaterfall]($UI.WavetableWaterfall$) | $MODULES.WavetableSynth$ | 3D waterfall display for wavetable morphing |
| [MatrixPeakMeter]($UI.MatrixPeakMeter$) | - | Multi-channel peak meter for any module |
| [PerformanceLabel]($UI.PerformanceLabel$) | - | CPU usage, RAM consumption, and voice count display |

---

### MIDI and Input

| Floating Tile | Related | Description |
|--------------|---------|-------------|
| [Keyboard]($UI.Keyboard$) | $API.Engine$ | Virtual MIDI keyboard with MPE support and custom graphics |
| [MidiOverlayPanel]($UI.MidiOverlayPanel$) | $MODULES.MidiPlayer$ | Piano roll and step sequencer overlay for MIDI Player |
| [MidiLearnPanel]($UI.MidiLearnPanel$) | $API.MidiAutomationHandler$ | Table of MIDI CC-learned controls with range editing |
| [MidiChannelList]($UI.MidiChannelList$) | - | MIDI channel selector panel |
| [MidiSources]($UI.MidiSources$) | - | Connected MIDI device selector |
| [ActivityLED]($UI.ActivityLED$) | - | MIDI activity indicator light |

---

### Modulation

| Floating Tile | Related | Description |
|--------------|---------|-------------|
| [ModulationMatrix]($UI.ModulationMatrix$) | $MODULES.MatrixModulator$ | Connection grid or slider matrix for modulation routing |
| [ModulationMatrixController]($UI.ModulationMatrixController$) | $MODULES.GlobalModulatorContainer$ | Drag-and-drop modulation source assignment |
| [MPEPanel]($UI.MPEPanel$) | $MODULES.MPEModulator$ | MPE modulator detection and configuration |
| [FrontendMacroPanel]($UI.FrontendMacroPanel$) | $API.MacroHandler$ | Macro parameter assignment management |

---

### Content

| Floating Tile | Related | Description |
|--------------|---------|-------------|
| [MarkdownPanel]($UI.MarkdownPanel$) | - | Embedded markdown documentation browser with TOC and search |
| [TooltipPanel]($UI.TooltipPanel$) | - | Displays tooltip text of the currently hovered component |
