---
title: UI Components
description: Reference for all HISE UI components - plugin components, floating tiles, styling, and the component data model

guidance:
  summary: >
    Complete reference for HISE UI components. Plugin components are scriptable
    controls (Button, Knob, ComboBox, Label, Panel, Image, Table, SliderPack,
    AudioWaveform, Viewport) created via Content.add*() in onInit. Floating tiles
    are pre-built panels (PresetBrowser, Keyboard, AudioAnalyser, CustomSettings,
    etc.) configured via JSON ContentType properties. UI components live in an XML
    data model that persists across script recompilation. Scripts act as a
    repeatable post-processing step that creates components and attaches logic.
    Styling via Look and Feel (LAF) paint routines or CSS stylesheets.
  concepts:
    - ui components reference
    - plugin components
    - floating tiles
    - component data model
    - look and feel
    - CSS styling
    - preset storage
    - plugin parameters
  complexity: advanced
---

UI components are the building blocks of every HISE plugin interface. They divide into two subsystems: [Plugin Components](/v2/reference/ui-components/components/) are scriptable controls that you create and configure from HiseScript, and [Floating Tiles](/v2/reference/ui-components/floating-tiles/) are pre-built panels for common tasks like preset browsing or audio analysis.

## The Component Data Model

UI components in HISE are not owned by the scripting layer. They live in a persistent XML data model that exists independently of any script. When you call `Content.addKnob("Vol", 0, 0)` in your `onInit` callback, HISE checks whether a component with that ID already exists in the XML tree. If it does, the existing component is reused with its current property values and state intact. If it does not, a new component entry is created.

This means that **recompiling a script does not recreate existing components**. Their property values, positions, colours, and parent-child relationships all persist in the XML tree across recompilations. The script compilation is a repeatable post-processing step that:

1. **Creates** new components if they do not already exist in the data model
2. **Attaches logic** to the components - control callbacks, custom paint routines, timer callbacks, and mouse handlers

![Component Data Model](/images/v2/reference/ui-components/component-data-model.svg)

The XML data model stores every component as a child element of `<ContentProperties>`, with all properties and the current value as XML attributes. This is the same representation you see when you copy a component's state to the clipboard in the HISE IDE.

The script layer operates on top of this model. The `onInit` callback can read and write properties via `component.get()` and `component.set()`, and the runtime callbacks (`onControl`, paint routines, timer callbacks) react to value changes and repaint the visual UI components. The important consequence is that the script does not define the interface state - it transforms and extends a declarative model that is managed by the engine.

The visual JUCE UI components (the actual rendered knobs, buttons, and panels you see on screen) are created from the XML data model. They reflect the current state of the model and update when properties or values change.

> [!Warning:Operations that invalidate the component tree] Certain structural changes - modifying the `parentComponent` property or removing components - invalidate all UI components. After invalidation, the visual components are rebuilt from the XML data model and a script recompilation is required to restore callbacks and paint routines.

### Implications for Development

This architecture has practical consequences for the development workflow:

- **Recompile freely**: You can iterate on script logic without losing UI state. Component positions, colours, and values survive recompilation.
- **IDE property editor**: The property editor in the HISE IDE writes directly to the XML data model, bypassing the script entirely. Changes made there persist even if the script does not set that property.
- **Script as post-processor**: Think of `onInit` as a setup pass that runs after the data model is loaded, not as a constructor that builds the interface from scratch. If a property was already set (by the IDE or a previous compilation), the script only overwrites it if it explicitly calls `component.set()`.

## Component Hierarchy

Every interface has an implicit root container called `Content`. Components are added as children of this root via factory methods like `Content.addKnob()` or `Content.addPanel()`. Components can be nested by setting the `parentComponent` property, which places the child inside another component (typically a Panel).

```javascript
const var bg = Content.addPanel("Background", 0, 0);
const var knob = Content.addKnob("Volume", 10, 10);
knob.set("parentComponent", "Background");
```

Z-order follows creation order: components created later render on top of earlier ones at the same hierarchy level.

## Types and Identification

Each component has a fixed **type** determined by the factory method used to create it (`addKnob`, `addButton`, `addPanel`, etc.) and a unique **string ID** that identifies it within the interface. The ID must be unique across the entire interface - duplicate IDs cause undefined behaviour.

```javascript
// Create components (onInit only)
const var knob = Content.addKnob("Volume", 0, 0);
const var btn  = Content.addButton("Bypass", 100, 0);

// Retrieve a reference to an existing component
const var ref = Content.getComponent("Volume");
```

The type cannot be changed after creation. If you need a different component type, remove the old one and create a new one (which triggers a full invalidation).

## Styling: LAF vs CSS

HISE provides two styling systems for visual customisation of UI components.

**Look and Feel (LAF)** uses JavaScript paint routines registered via a [ScriptLookAndFeel]($API.ScriptLookAndFeel$) object. Each component type has one or more named LAF functions (e.g. `drawRotarySlider` for knobs, `drawToggleButton` for buttons). The paint routine receives a [Graphics]($API.Graphics$) object and a data object with the component's current state, giving full programmatic control over rendering.

```javascript
const var laf = Content.createLocalLookAndFeel();
laf.registerFunction("drawRotarySlider", function(g, obj) {
    // custom drawing code
});
knob.setLocalLookAndFeel(laf);
```

**CSS** uses stylesheet-based styling with selectors and pseudo-classes, closer to web development. Stylesheets are registered per component and support standard CSS properties for colours, borders, backgrounds, and layout.

```javascript
const var ss = Content.createLocalLookAndFeel();
ss.setInlineStyleSheet("
    .scriptknob {
        background: #333;
        border-radius: 50%;
    }
    .scriptknob:hover { background: #444; }
");
knob.setLocalLookAndFeel(ss);
```

Both systems can coexist on different components within the same interface. Each component's reference page documents its available LAF function names and CSS selectors.

## Floating Tiles

Floating tiles are pre-built UI panels for common plugin features like preset browsing, virtual keyboards, audio analysis, and MIDI configuration. Unlike plugin components, they are not scripted from scratch - they are configured declaratively via JSON properties.

A floating tile is added to the interface as a [ScriptFloatingTile]($API.ScriptFloatingTile$) component, which acts as a host container. The `ContentType` property selects which panel to display:

```javascript
const var ft = Content.addFloatingTile("Browser", 0, 0);
ft.set("ContentType", "PresetBrowser");
```

See [Floating Tiles](/v2/reference/ui-components/floating-tiles/) for the full reference of available tile types and their JSON properties.

## Component Types

**[Plugin Components](/v2/reference/ui-components/components/)** - Scriptable interface controls with callbacks, custom paint routines, and host automation support. These are the primary building blocks for custom plugin UIs.

**[Floating Tiles](/v2/reference/ui-components/floating-tiles/)** - Pre-built functional panels configured via JSON properties. Use these for standard features like preset management, keyboard display, and audio visualisation.
