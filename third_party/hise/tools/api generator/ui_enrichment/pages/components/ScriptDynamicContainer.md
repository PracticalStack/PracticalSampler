---
title: "Dynamic Container"
description: "Data-driven container that creates child UI components from JSON at runtime for dynamic FX chains, preset browsers, and other variable-structure interfaces."
componentId: "ScriptDynamicContainer"
componentType: "plugin-component"
screenshot: ""
llmRef: |
  ScriptDynamicContainer (UI component)
  Create via: Content.addDynamicContainer("name", x, y)
  Scripting API: $API.ScriptDynamicContainer$, $API.ContainerChild$

  Data-driven container that dynamically creates and manages child UI components
  from JSON. Use it when the number or type of controls is not known at compile
  time - dynamic FX chains, preset browsers, sample lists, or any UI that must
  be built and rebuilt from data at runtime.

  Call setData(jsonArray) to build the component tree. Returns a ContainerChild
  root reference for hierarchy traversal. Register setValueCallback(f(id, value))
  after setData to listen for child value changes. Each setData() call invalidates
  all previous ContainerChild references.

  Supported child types (with static equivalent):
    Button (ScriptButton), Slider (ScriptSlider), ComboBox (ScriptComboBox),
    Label (ScriptLabel), Panel (ScriptPanel), FloatingTile (ScriptFloatingTile),
    TableEditor (ScriptTable), SliderPack (ScriptSliderPack), AudioFile (ScriptAudioWaveform)

  Container/display-only types (no static equivalent):
    DragContainer: drag-reorderable children with EffectProcessorChain connection
    Viewport: CSS flexbox layout container with scrolling
    TextBox: markdown text display

  Customisation:
    LAF: none (children use their own component type's LAF)
    CSS: none on container (children styled via their type selectors)
    Filmstrip: no

seeAlso: []
commonMistakes:
  - title: "Calling setValueCallback before setData"
    wrong: "Calling dc.setValueCallback(f) before dc.setData(json)"
    right: "Call dc.setData(json) first, then dc.setValueCallback(f)"
    explanation: "The value callback monitors the internal value store, which does not exist until setData() creates it. Calling setValueCallback() first silently does nothing."
  - title: "Using stale ContainerChild references after setData"
    wrong: "Storing a ContainerChild reference and reusing it after calling setData() again"
    right: "Re-obtain all references from the new setData() return value. Check ref.isValid() if uncertain."
    explanation: "Each setData() call rebuilds the entire component tree and permanently invalidates all previous ContainerChild references. Using a stale reference throws a script error."
  - title: "Expecting setValue to trigger the control callback"
    wrong: "Calling cc.setValue(0.5) and expecting the control callback to fire"
    right: "Call cc.setValue(0.5) then cc.changed() to trigger the callback and visual refresh"
    explanation: "setValue() writes silently to the value store. You must call changed() afterward to fire the control callback and update the visual state."
  - title: "Reading properties via dot syntax expecting defaults"
    wrong: "var text = cc.text; // may be undefined if not set in JSON"
    right: "var text = cc.get(\"text\"); // returns the default value if not set"
    explanation: "Dot-read syntax (cc.text) returns the raw ValueTree property which may be undefined. Use cc.get(\"propertyName\") for default-value fallback."
  - title: "Treating removeFromParent as synchronous"
    wrong: "cc.removeFromParent(); doSomethingWith(parent.getNumChildComponents());"
    right: "Use a timer or defer the follow-up logic - removal is async via SafeAsyncCall"
    explanation: "removeFromParent() and removeAllChildren() are deferred. The component tree is not updated immediately upon return. getNumChildComponents() may still return the old count right after the call."
---

ScriptDynamicContainer is a data-driven UI container that creates child components from JSON at runtime. The container itself has no visual representation - it is purely a management layer. All rendering is done by the child components it creates.

The standard HISE interface uses a [persistent XML data model](/v2/reference/ui-components/) where components are created once in `onInit` via `Content.add*()` factory methods and exist for the plugin's lifetime. The dynamic container introduces a second, parallel data model within the interface - its own component tree built from JSON, with components that can be created and destroyed at runtime.

| | Main Interface | Dynamic Container |
|---|---|---|
| **Root** | `Content` | `ScriptDynamicContainer` |
| **Data model** | Persistent XML (`<ContentProperties>`) | JSON-driven ValueTree |
| **Create components** | `Content.addButton()`, `Content.addKnob()`, ... | `dc.setData(jsonArray)`, `cc.addChildComponent(json)` |
| **Component references** | [ScriptButton]($API.ScriptButton$), [ScriptSlider]($API.ScriptSlider$), [ScriptPanel]($API.ScriptPanel$), ... | [ContainerChild]($API.ContainerChild$) with `type: "Button"`, `"Slider"`, `"Panel"`, ... |
| **Lifetime** | Static - persist across recompilation | Dynamic - created/destroyed at runtime |
| **Value persistence** | `saveInPreset`, DAW automation, macros | Internal value model, `addStateToUserPreset()` |

> [!Tip:Consider ScriptedViewport for fixed-structure data] If your dynamic UI displays a data set with a uniform structure (e.g. a file list or parameter table), [ScriptedViewport]($UI.Components.ScriptedViewport$) in table mode may be a simpler alternative. The dynamic container is best suited for heterogeneous layouts where each item can be a different component type.

Use the dynamic container when the UI structure is not fixed at compile time: draggable effect chains, file browsers with custom item rendering, dynamic preset selectors, or any UI where controls need to be created and destroyed based on runtime data.

## Getting Started

### Basic setup

```javascript
// 1. Create the container
const var dc = Content.addDynamicContainer("MyContainer", 0, 0);
dc.setPosition(0, 0, 500, 300);

// 2. Define components as JSON
const var data = [
{
    "id": "Volume",
    "type": "Slider",
    "text": "Volume",
    "min": 0.0,
    "max": 1.0,
    "defaultValue": 0.8,
    "style": "Knob",
    "x": 10, "y": 10, "width": 128, "height": 48
},
{
    "id": "Bypass",
    "type": "Button",
    "text": "Bypass",
    "x": 150, "y": 10, "width": 128, "height": 32
}];

// 3. Build the tree - returns a ContainerChild root reference
//    (pass a single object instead of an array to get a direct
//     reference to that component rather than a root tree node)
const var rc = dc.setData(data);

// 4. Register the value callback AFTER setData
dc.setValueCallback(function(id, value)
{
    Console.print(id + " = " + value);
});
```

### Accessing and modifying child components

```javascript
// By ID (recursive search)
const var volumeKnob = rc.getComponent("Volume");

// By wildcard pattern (NOT regex - uses * and ? matching)
const var allSliders = rc.getAllComponents("*Slider*");

// Traverse the hierarchy
var numChildren = rc.getNumChildComponents();
var parent = volumeKnob.getParent();

// Add a new child at runtime
var newBtn = rc.addChildComponent({
    "id": "Mute", "type": "Button", "text": "Mute",
    "x": 300, "y": 10, "width": 128, "height": 32
});

// Remove a child (deferred - not immediate)
newBtn.removeFromParent();
```

## Common JSON Properties

Every child type accepts these properties. Only `id` and `type` are required - all others fall back to defaults. These correspond to the common properties shared by all static plugin components.

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `id` | String | `""` | Unique identifier for the component (required) |
| `type` | String | `""` | Component type name (required) |
| `text` | String | `""` | Display text / label |
| `x`, `y`, `width`, `height` | int | `0`, `0`, `128`, `50` | Position and size in pixels |
| `visible`, `enabled` | bool | `true` | Display and interaction state |
| `tooltip` | String | `""` | Hover tooltip text |
| `defaultValue` | double | `0.0` | Initial value and double-click reset target |
| `useUndoManager` | bool | `false` | Enable undo/redo for value changes |
| `parentComponent` | String | `""` | Parent component ID (auto-set from JSON hierarchy) |
| `processorId` | String | `""` | Processor to connect to |
| `parameterId` | String/int | `""` | Parameter index or name on the connected processor |
| `bgColour`, `itemColour`, `itemColour2`, `textColour` | colour | | Colour properties (same defaults as static components) |
| `class` | String | `""` | CSS class name for styling |
| `elementStyle` | String | `""` | Inline CSS style string |

Legacy type names (`ScriptButton`, `ScriptSlider`, `ScriptComboBox`, `ScriptLabel`, `ScriptPanel`, `ScriptViewport`) are accepted and auto-converted.

## Child Types with Plugin Component Equivalents

These child types correspond to existing HISE plugin components. They share the same visual appearance and CSS/LAF selectors, but differ in their scripting API - you interact with them through `ContainerChild` methods rather than the static ScriptComponent methods.

### Button

![Button](/images/v2/reference/ui-components/button.png)

**See also:** $UI.Components.ScriptButton$ -- static plugin component equivalent

Toggle or momentary button. Value is a boolean (toggle state).

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `isMomentary` | bool | `false` | `true` for momentary (press-and-hold) behavior |
| `radioGroupId` | int | `0` | Radio group ID - only one button in the group can be active |
| `setValueOnClick` | bool | `false` | `true` to fire on mouse down instead of mouse up |

| Feature | Dynamic | Static |
|---------|---------|--------|
| Toggle / momentary | ✓ | ✓ |
| Radio groups | ✓ (scoped to container root) | ✓ (interface-wide) |
| Filmstrip rendering | ✗ | ✓ |
| Control callback args | 1 (value) | 2 (component, value) |
| saveInPreset | ✗ (use `addStateToUserPreset`) | ✓ |
| CSS / LAF | ✓ (identical selectors) | ✓ |

### Slider

![Slider](/images/v2/reference/ui-components/knob.png)

**See also:** $UI.Components.ScriptSlider$ -- static plugin component equivalent

Knob or linear slider with configurable range and display mode. Supports direct processor parameter connection via `processorId` and `parameterId`.

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `min` | double | `0.0` | Minimum value |
| `max` | double | `1.0` | Maximum value |
| `stepSize` | double | `0.01` | Value increment step |
| `middlePosition` | double | `-10` | Skew midpoint (`-10` = no skew) |
| `mode` | String | `""` | Value mode: `"Frequency"`, `"Time"`, `"Decibel"`, etc. |
| `suffix` | String | `""` | Text suffix appended to value display |
| `style` | String | `"Knob"` | Visual style: `"Knob"`, `"Horizontal"`, `"Vertical"`, `"Range"` |
| `showValuePopup` | bool | `false` | Show value popup on drag |
| `processorId` | String | `""` | Processor ID for direct parameter connection |
| `parameterId` | String/int | `""` | Parameter name or index on the connected processor |

| Feature | Dynamic | Static |
|---------|---------|--------|
| Range / mode / step | ✓ | ✓ |
| Processor connection | ✓ (via JSON `processorId`) | ✓ (via property panel) |
| Modulation ring display | ✗ | ✓ |
| mouseSensitivity | ✗ | ✓ |
| dragDirection | ✗ | ✓ |
| Film strip | ✓ | ✓ |
| Control callback args | 1 (value) | 2 (component, value) |
| CSS / LAF | ✓ (identical selectors) | ✓ |

```javascript
// Processor-connected slider example
const var data = [{
    "id": "Attack",
    "type": "Slider",
    "text": "Attack",
    "processorId": "SimpleEnvelope1",
    "parameterId": "Attack",
    "min": 0.0,
    "max": 20000.0,
    "mode": "Time",
    "suffix": " ms",
    "style": "Knob",
    "x": 10, "y": 10, "width": 128, "height": 48
}];
```

Use `updateValueFromProcessorConnection(true)` on a ContainerChild to force a refresh from the processor.

### ComboBox

![ComboBox](/images/v2/reference/ui-components/combobox.png)

**See also:** $UI.Components.ScriptComboBox$ -- static plugin component equivalent

Dropdown selector. Value is the selected item ID (1-based index).

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `items` | String | `""` | Newline-separated list of items |
| `useCustomPopup` | bool | `false` | Use HISE's custom popup menu renderer |
| `popupMenuAlign` | bool | `false` | Align popup to component bounds |

| Feature | Dynamic | Static |
|---------|---------|--------|
| Items (newline-separated) | ✓ | ✓ |
| Custom popup | ✓ | ✓ |
| popupMenuColumns | ✗ | ✓ |
| Value (1-based index) | ✓ | ✓ |
| CSS / LAF | ✓ (identical selectors) | ✓ |

### Label

![Label](/images/v2/reference/ui-components/label.png)

**See also:** $UI.Components.ScriptLabel$ -- static plugin component equivalent

Text input or display field.

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `editable` | bool | `true` | Whether the user can type in the label |
| `multiline` | bool | `false` | Allow multi-line text input |
| `updateEachKey` | bool | `false` | Fire value callback on each keystroke rather than on focus loss |

| Feature | Dynamic | Static |
|---------|---------|--------|
| Editable / multiline | ✓ | ✓ |
| updateEachKey | ✓ | ✓ |
| fontName / fontSize / fontStyle | ✗ (use CSS) | ✓ |
| Value (text string) | ✓ | ✓ |
| CSS | ✓ (identical selectors) | ✓ |

### Panel

![Panel](/images/v2/reference/ui-components/panel.png)

**See also:** $UI.Components.ScriptPanel$ -- static plugin component equivalent

Custom drawable panel. The most flexible child type - use it when none of the others fit.

| Feature | Dynamic | Static |
|---------|---------|--------|
| Custom paint routine | ✓ (`this` = ContainerChild) | ✓ (`this` = ScriptPanel) |
| Mouse callback | ✗ | ✓ |
| Timer callback | ✗ | ✓ |
| Animation (Lottie, etc.) | ✗ | ✓ |
| Drag and drop | ✗ | ✓ |
| Popup menu | ✗ | ✓ |
| Child panels | ✗ (use nested JSON) | ✓ |
| CSS | ✓ (identical selectors) | ✓ |

To draw custom content, call `setPaintRoutine()` on the ContainerChild reference. Inside the callback, `this` refers to the ContainerChild:

```javascript
var panel = rc.getComponent("MyPanel");

panel.setPaintRoutine(function(g)
{
    g.setColour(this.get("bgColour"));
    g.fillRoundedRectangle(this.getLocalBounds(1), 2);
    g.setColour(Colours.white);
    g.drawAlignedText(this.get("text"), this.getLocalBounds(0), "centred");
});
```

### FloatingTile

![FloatingTile](/images/v2/reference/ui-components/floating-tile.png)

**See also:** $UI.Components.ScriptFloatingTile$ -- static plugin component equivalent

Embeds a HISE floating tile widget (Keyboard, PresetBrowser, AHDSRGraph, etc.) as a dynamic child.

The floating tile configuration data must be provided in a `FloatingTileData` property at the top level of the `setData()` object. The child's `id` maps to a key in this object.

| Feature | Dynamic | Static |
|---------|---------|--------|
| Content types | ✓ (identical) | ✓ |
| Configuration | via `FloatingTileData` JSON | via `setContentData()` |
| LAF / CSS | ✓ (identical per content type) | ✓ |

```javascript
const var data = {
    "ContentProperties": [{
        "id": "Keys",
        "type": "FloatingTile",
        "x": 0, "y": 0, "width": 500, "height": 100
    }],
    "FloatingTileData": {
        "Keys": {
            "ContentType": "Keyboard",
            "KeyWidth": 14
        }
    }
};

dc.setData(data);
```

### TableEditor

![TableEditor](/images/v2/reference/ui-components/table.png)

**See also:** $UI.Components.ScriptTable$ -- static plugin component equivalent

Displays and edits a table curve from a processor's complex data.

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `processorId` | String | `""` | ID of the processor holding the table data |
| `index` | int | `0` | Which table on the processor to display |

| Feature | Dynamic | Static |
|---------|---------|--------|
| Complex data connection | ✓ (via JSON `processorId` + `index`) | ✓ (via property panel) |
| Table popup function | ✗ | ✓ |
| Custom colours property | ✗ | ✓ |
| CSS / LAF | ✓ (identical selectors) | ✓ |

```javascript
rc.addChildComponent({
    "id": "Table1",
    "type": "TableEditor",
    "processorId": "ScriptFX1",
    "index": 0,
    "width": 300, "height": 150
});
```

### SliderPack

![SliderPack](/images/v2/reference/ui-components/sliderpack.png)

**See also:** $UI.Components.ScriptSliderPack$ -- static plugin component equivalent

Displays and edits a slider pack (multi-slider array) from a processor's complex data.

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `processorId` | String | `""` | ID of the processor holding the slider pack data |
| `index` | int | `0` | Which slider pack on the processor to display |

| Feature | Dynamic | Static |
|---------|---------|--------|
| Complex data connection | ✓ (via JSON `processorId` + `index`) | ✓ (via property panel) |
| setWidthArray | ✗ | ✓ |
| setAllValueChangeCausesCallback | ✗ | ✓ |
| CSS / LAF | ✓ (identical selectors) | ✓ |

```javascript
rc.addChildComponent({
    "id": "Pack1",
    "type": "SliderPack",
    "processorId": "ScriptFX1",
    "index": 0,
    "width": 300, "height": 150
});
```

### AudioFile

![AudioFile](/images/v2/reference/ui-components/audio-waveform.png)

**See also:** $UI.Components.ScriptAudioWaveform$ -- static plugin component equivalent

Displays an audio waveform from a processor's audio file data.

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `processorId` | String | `""` | ID of the processor holding the audio file data |
| `index` | int | `0` | Which audio file on the processor to display |

| Feature | Dynamic | Static |
|---------|---------|--------|
| Complex data connection | ✓ (via JSON `processorId` + `index`) | ✓ (via property panel) |
| Playback position | ✗ | ✓ (`setPlaybackPosition`) |
| Range display | ✗ | ✓ (`getRangeStart/End`) |
| CSS / LAF | ✓ (identical selectors) | ✓ |

```javascript
rc.addChildComponent({
    "id": "Wave1",
    "type": "AudioFile",
    "processorId": "ScriptFX1",
    "index": 0,
    "width": 300, "height": 150
});
```

## Container and Display Types

These child types have no static plugin component equivalent. They exist only within ScriptDynamicContainer.

### DragContainer

A container whose children can be reordered by dragging. Optionally connects to an `EffectProcessorChain` to synchronise the visual order with the audio processing order.

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `isVertical` | bool | `true` | `true` for vertical stacking, `false` for horizontal |
| `animationSpeed` | double | `150` | Animation duration in ms when items snap to position. `0` for instant. |
| `dragMargin` | int | `3` | Pixel margin around the drag area |
| `processorId` | String | `""` | ID of an EffectProcessorChain to sync reordering with |
| `min` | int | `0` | Start index in the effect chain's slot range |
| `max` | int | `0` | End index in the effect chain's slot range |
| `polyFX` | bool | `false` | `true` for polyphonic effects (VoiceEffectProcessor) |

**CSS:** Styled via its `#id` selector. Children use their own type selectors.

When children are dragged and released, the container:
1. Sorts children by their visual position
2. Fires the value callback with an array of effect indices in the new order
3. If connected to an EffectProcessorChain, automatically reorders the audio processing chain

The `text` property of each DragContainer child is used as the effect name when connecting to `HotswappableProcessor` slots.

```javascript
const var dc = Content.addDynamicContainer("FXChain", 0, 0);
dc.setPosition(0, 0, 600, 400);

// Create the drag container connected to an effect chain.
// min/max define the slot range within the chain.
const var data = {
    "id": "FXList",
    "width": 600,
    "height": 400,
    "type": "DragContainer",
    "isVertical": true,
    "animationSpeed": 150,
    "dragMargin": 3,
    "processorId": "MyEffectChain",
    "min": 0,
    "max": 4
};

const var rc = dc.setData(data);

const var panels = [];

// Add effect slots as children of the DragContainer
panels.push(rc.addChildComponent({
    "id": "EQ",
    "type": "Panel",
    "text": "Parametric EQ",
    "bgColour": Colours.teal,
    "width": 580, "height": 80
}));

panels.push(rc.addChildComponent({
    "id": "Comp",
    "type": "Panel",
    "text": "Compressor",
    "bgColour": Colours.firebrick,
    "width": 580, "height": 80
}));

panels.push(rc.addChildComponent({
    "id": "Reverb",
    "type": "Panel",
    "text": "Reverb",
    "bgColour": Colours.darkgoldenrod,
    "width": 580, "height": 80
}));

// Add a paint routine to the panels - just render the text and background colour
for(p in panels)
{
    p.setPaintRoutine(function(g)
    {
        g.setColour(this.get("bgColour"));
        g.fillRoundedRectangle(this.getLocalBounds(1), 2);
        g.setColour(Colours.white);
        g.drawAlignedText(this.get("text"), this.getLocalBounds(0), "centred");
    });
}
```

### Viewport

A CSS flexbox layout container. Children are automatically arranged using CSS flexbox rules rather than absolute positioning. Wraps a `FlexboxViewport` which provides scrolling when the content exceeds the container bounds.

**CSS:** `div` or `#id` selector. Children are arranged as flex items.

Use the Viewport type for scrollable lists, grids, or any layout where children should flow automatically:

```javascript
const var dc = Content.addDynamicContainer("Browser", 0, 0);
dc.setPosition(0, 0, 100, 200);

const var rc = dc.setData({
    "id": "Grid",
    "type": "Viewport",
    "width": 100,
    "height": 200,
    "ContentProperties": items
});

// Build a grid of items using Viewport (flexbox)
const var items = [];

for (i = 0; i < 40; i++)
{
    items.push(rc.addChildComponent({
        "id": "Item" + i,
        "type": "Button",
        "text": "Preset " + (i + 1),
        "width": 120,
        "height": 20,
        "radioGroupId": 200,
        "class": "preset-item"
    }));
}

// Style with CSS - children inherit the .scriptviewport context
const var laf = Content.createLocalLookAndFeel();
laf.setInlineStyleSheet("

* {}

#Grid {
    display: flex;
    flex-direction: row;
    flex-wrap: wrap;
    gap: 0px;  
}
button {
    background: #444;
    color: white;
    border-radius: 4px;
    margin: 1px;
}

button:checked
{
    background: red;
}

button:hover {
    background: #666;
}
");

dc.setLocalLookAndFeel(laf);
```

### TextBox

A read-only multi-line text display that renders markdown content. Use it for help text, descriptions, or any formatted text that needs to be part of a dynamic layout.

The `text` property accepts markdown-formatted strings.

**CSS:** Style with the `p` selector for paragraph text styling.

```javascript
rc.addChildComponent({
    "id": "Help",
    "type": "TextBox",
    "text": "## Help\nDrag the effects above to reorder the signal chain.",
    "width": 300, "height": 100
});
```

**See also:** {placeholder - populated during cross-reference post-processing}
