---
title: Plugin Components
description: All scriptable UI components for building HISE plugin interfaces - values, callbacks, automation, and categorized component list
---

Plugin components are the scriptable UI controls that make up a HISE instrument's interface. Each is created via a `Content.add*()` factory method in `onInit` and returns a typed script reference for runtime interaction. Common properties shared by all components are documented on each component's individual reference page.

## Creating Components

Components are created in the `onInit` callback using factory methods on the `Content` namespace. Each method takes a unique string ID and an initial x/y position:

```javascript
const var knob = Content.addKnob("Volume", 10, 10);
const var btn  = Content.addButton("Bypass", 120, 10);
const var pan  = Content.addPanel("Background", 0, 0);
```

The ID must be unique across the entire interface. Creating a component with an ID that already exists in the XML data model reuses the existing component without resetting its properties or value.

> [!Tip:Use Content.getComponent for existing components] `Content.add*()` calls reposition the component to the given x/y coordinates, reverting any manual adjustments made in the Interface Designer. If a component already exists in the data model, use `Content.getComponent("id")` instead to obtain a script reference without modifying its position.

## Values and Preset Storage

[Automatable components](#automatable-components) store a numeric value accessed via `getValue()` and `setValue()`. The value range depends on the component type - buttons use 0/1, knobs have a configurable range via `min`/`max`/`stepSize`, and combo boxes use a 1-based index into their items list.

An important distinction: `setValue()` updates the stored value but does **not** fire the control callback. To trigger the callback programmatically, call `changed()` after `setValue()`:

```javascript
knob.setValue(0.5);     // updates value, no callback
knob.changed();         // fires the control callback
```

The `saveInPreset` property controls whether a component's value is stored in user presets. When a preset is loaded, all components with `saveInPreset` enabled get their values restored and their control callbacks fire. Components with `saveInPreset` disabled keep their current value and receive no callback.

| Type | saveInPreset default |
|------|---------------------|
| Knob, Button, ComboBox | `true` |
| Panel, Image, Viewport, Label | `false` |
| Table, SliderPack, AudioWaveform | `false` |

The `defaultValue` property sets the initial value used at first creation and when loading a preset that has no stored value for this component. Knobs also use it as the double-click reset value.

## Controlling Parameters

The simplest way to connect a component to an audio module parameter is the `processorId` and `parameterId` property pair. When set, the component's value directly controls the specified parameter using the component's own range:

```javascript
knob.set("processorId", "SimpleGain1");
knob.set("parameterId", "Gain");
```

For anything more complex - controlling multiple parameters, applying value transformations, or conditional logic - use the control callback. It fires when a component's value changes through user interaction or a programmatic `changed()` call:

```javascript
inline function onControl(component, value)
{
    if (component == knob)
        effect.setAttribute(effect.Gain, value);

    if (component == btn)
        effect.setBypassed(1 - value);
};

Content.getComponent("Volume").setControlCallback(onControl);
Content.getComponent("Bypass").setControlCallback(onControl);
```

> [!Tip:Switch on component reference, not ID] Compare against the component reference variable (`component == knob`) rather than checking `component.get("id")`. Reference comparison is faster and less error-prone.

## Plugin Parameters and MIDI Learn

For [automatable components](#automatable-components), the `isPluginParameter` property exposes a component to the DAW for host automation. Use `pluginParameterName` to set the display name in the host. When one parameter causes other parameters to change (e.g. a sync button that modifies delay time), set `isMetaParameter` to `true` for correct behaviour in Logic Pro.

Components with `enableMidiLearn` set to `true` allow users to right-click and assign a MIDI CC controller. The learned assignment is saved with user presets.

> [!Warning:MIDI learn requires saveInPreset] The `enableMidiLearn` property is silently ignored when `saveInPreset` is `false`. Both must be enabled for MIDI learn to function.

Use the [MidiLearnPanel]($UI.MidiLearnPanel$) floating tile to give users a management interface for all MIDI-learned controls. For programmatic control over MIDI assignments, use the [MidiAutomationHandler]($API.MidiAutomationHandler$) scripting API.

## Components

### Automatable Components

These components store a value, save in presets by default, and can be exposed as plugin parameters for host automation. They are the primary controls for user interaction.

| Component | Scripting API | Description |
|-----------|--------------|-------------|
| [Knob]($UI.ScriptSlider$) | [ScriptSlider]($API.ScriptSlider$) | Rotary, horizontal, or vertical slider with configurable range and filmstrip support |
| [Button]($UI.ScriptButton$) | [ScriptButton]($API.ScriptButton$) | Toggle or momentary button with filmstrip support and radio group mutual exclusion |
| [ComboBox]($UI.ScriptComboBox$) | [ScriptComboBox]($API.ScriptComboBox$) | Drop-down selector with a string items list |

### Read-Only Components

Display-only components that do not save in presets and are not automatable by default. Use these for labels, images, and visual feedback.

| Component | Scripting API | Description |
|-----------|--------------|-------------|
| [Label]($UI.ScriptLabel$) | [ScriptLabel]($API.ScriptLabel$) | Editable or read-only text display |
| [Image]($UI.ScriptImage$) | [ScriptImage]($API.ScriptImage$) | Static image display with alpha and colour tinting |
| [Panel]($UI.ScriptPanel$) | [ScriptPanel]($API.ScriptPanel$) | General-purpose component with custom paint, mouse handling, and timer callbacks |

> [!Tip:ScriptPanel lives in both worlds] ScriptPanel defaults to `saveInPreset = false` and is listed here as a read-only display component, but it can be made automatable by setting `saveInPreset` to `true`. This makes it the most versatile component type - it can serve as a display canvas, a custom control, or a layout container depending on configuration.

### Container Components

Components that host child content - embedded web views, dynamically generated component lists, floating tile panels, or scrollable viewports.

| Component | Scripting API | Description |
|-----------|--------------|-------------|
| [Panel]($UI.ScriptPanel$) | [ScriptPanel]($API.ScriptPanel$) | Can host child components via `parentComponent` for layout grouping |
| [Floating Tile]($UI.ScriptFloatingTile$) | [ScriptFloatingTile]($API.ScriptFloatingTile$) | Hosts a pre-built [Floating Tile](/v2/reference/ui-components/floating-tiles/) panel configured via JSON |
| [Viewport]($UI.ScriptedViewport$) | [ScriptedViewport]($API.ScriptedViewport$) | Scrollable list with custom item rendering |
| [Dynamic Container]($UI.ScriptDynamicContainer$) | [ScriptDynamicContainer]($API.ScriptDynamicContainer$) | Data-driven container that creates child components from JSON at runtime |
| WebView | [ScriptWebView]($API.ScriptWebView$) | Embedded web browser for HTML/CSS/JS-based UI |

### Complex Data Components

These components bridge the UI and audio processing layers. They connect to the complex data system through typed processor references, allowing the user to edit data that audio modules read during processing.

| Component | Scripting API | Description |
|-----------|--------------|-------------|
| [Table]($UI.ScriptTable$) | [ScriptTable]($API.ScriptTable$) | Editable lookup curve, connects to [TableProcessor]($API.TableProcessor$) |
| [SliderPack]($UI.ScriptSliderPack$) | [ScriptSliderPack]($API.ScriptSliderPack$) | Array of sliders, connects to [SliderPackProcessor]($API.SliderPackProcessor$) |
| [Audio Waveform]($UI.ScriptAudioWaveform$) | [ScriptAudioWaveform]($API.ScriptAudioWaveform$) | Waveform display, connects to [AudioSampleProcessor]($API.AudioSampleProcessor$) |
