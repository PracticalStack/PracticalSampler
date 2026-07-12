# ScriptDynamicContainer -- Class Analysis

## Brief
Data-driven UI container that dynamically creates and manages child components from JSON with value callbacks and preset integration.

## Purpose
ScriptDynamicContainer is a UI component that creates a hierarchy of child components from a JSON data model at runtime. Unlike traditional ScriptComponent children that are created individually via `Content.addXXX()` during onInit, ScriptDynamicContainer builds its entire component tree from structured data passed to `setData()`. The container returns `ContainerChild` reference objects that provide property access via dot syntax, hierarchy traversal, custom paint routines, value callbacks, child add/remove notifications, and user preset state serialization. It bridges the gap between static interface design and dynamic, data-driven UIs.

## Details

### Architecture Overview

ScriptDynamicContainer has a two-layer design:

1. **Container layer** (ScriptDynamicContainer itself) -- A standard ScriptComponent with only 2 own API methods (`setData`, `setValueCallback`). It acts as a thin wrapper that creates and owns a `dyncomp::Data` model.

2. **Reference layer** (ContainerChild) -- The primary scripting handle returned by `setData()`. ContainerChild objects provide 28 methods for property access, hierarchy traversal, value management, paint routines, serialization, and user preset integration.

### Data Model (dyncomp::Data)

The data model maintains two separate ValueTrees:

- **Data tree** -- Hierarchical component structure with properties (id, type, text, position, colours, etc.)
- **Values tree** -- Flat key-value store where each component's value is stored by its `id`

This separation means component properties (layout, appearance) and component values (user interaction state) are managed independently. Value callbacks listen to the Values tree, while property changes affect the Data tree.

### Dynamic Component Types

The container supports these child component types (specified via the `type` property in JSON):

| Type | Description |
|------|-------------|
| `"Button"` | Toggle or momentary button |
| `"Slider"` | Knob/slider with mode, range, and filmstrip support |
| `"ComboBox"` | Dropdown selector with items |
| `"Label"` | Text input/display |
| `"Panel"` | Custom drawable panel (supports paint routines) |
| `"FloatingTile"` | Embedded HISE floating tile widget |
| `"DragContainer"` | Drag-reorderable container (connects to FX chains) |
| `"Viewport"` | Flexbox layout container |
| `"TextBox"` | Multi-line text display |
| `"TableEditor"` | Table curve editor (complex data) |
| `"SliderPack"` | Multi-slider array (complex data) |
| `"AudioFile"` | Audio waveform display (complex data) |

Legacy type names (`ScriptButton`, `ScriptSlider`, `ScriptComboBox`, `ScriptLabel`, `ScriptPanel`, `ScriptViewport`) are accepted and converted automatically. See `setData()` for the full JSON property list accepted per component type.

### ContainerChild Property System

ContainerChild uses a separate property system from ScriptComponent. Properties are validated against `dyncomp::dcid::Helpers::isValidProperty()` and support dot-assignment syntax (`ref.text = "Hello"`).

Key properties include:
- **Common:** `id`, `type`, `text`, `enabled`, `visible`, `tooltip`, `defaultValue`, `useUndoManager`
- **Position:** `x`, `y`, `width`, `height` (also settable via `setBounds()`)
- **CSS:** `class`, `elementStyle`
- **Button:** `isMomentary`, `radioGroupId`, `setValueOnClick`
- **Slider:** `min`, `max`, `middlePosition`, `stepSize`, `mode`, `suffix`, `style`, `showValuePopup`
- **ComboBox:** `items`, `useCustomPopup`, `popupMenuAlign`
- **Label:** `editable`, `multiline`, `updateEachKey`
- **Visual:** `filmstripImage`, `numStrips`, `isVertical`, `scaleFactor`
- **Connection:** `processorId`, `parameterId`
- **Colours:** `bgColour`, `itemColour`, `itemColour2`, `textColour`

### ContainerChild Validity

ContainerChild references can become invalid when:
- The parent container's `setData()` is called again (invalidates all previous references)
- The component is removed via `removeFromParent()`
- The parent container is destroyed

All mutating methods check validity first and throw a script error if the reference is invalid. Use `isValid()` to check before operating on a reference that may have been invalidated. See `setData()` for invalidation details.

### User Preset Integration

ContainerChild supports user preset state persistence via `addStateToUserPreset()`. When enabled:
- The component tree and values are serialized to Base64 (zstd-compressed)
- Restored automatically on user preset load
- `resetValueToDefault()` removes all children and resets to `defaultValue`

### Deactivated Base Properties

ScriptDynamicContainer deactivates 16 of the standard ScriptComponent properties: `macroControl`, `isPluginParameter`, `min`, `max`, `defaultValue`, `pluginParameterName`, `text`, `tooltip`, `processorId`, `parameterId`, `isMetaParameter`, `linkedTo`, `automationId`, `deferControlCallback`, `pluginParameterGroup`, `saveInPreset`. The container does not participate in the preset/parameter/macro system -- values are managed internally.

## obtainedVia
`Content.addDynamicContainer(name, x, y)` -- creates during onInit.

## minimalObjectToken
dc

## Constants
None. ScriptDynamicContainer has no `addConstant()` calls.

## Dynamic Constants
None.

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| `dc.setValueCallback(onValue)` before `dc.setData(json)` | Call `dc.setData(json)` first, then `dc.setValueCallback(onValue)` | `setValueCallback` requires `setData()` to have been called first -- the callback listens to the data model's Values tree, which does not exist until `setData()` creates it. |
| Storing a ContainerChild reference and using it after a new `setData()` call | Re-obtain references after each `setData()` call, or check `ref.isValid()` | Calling `setData()` invalidates all previously returned ContainerChild references. Using an invalid reference throws a script error. |

## codeExample
```javascript
const var dc = Content.addDynamicContainer("MyContainer", 0, 0);
dc.setPosition(0, 0, 500, 300);

const var data = [
{
    "id": "Knob1",
    "type": "Slider",
    "text": "Volume",
    "min": 0.0,
    "max": 1.0,
    "x": 10, "y": 10, "width": 128, "height": 48
},
{
    "id": "Btn1",
    "type": "Button",
    "text": "Bypass",
    "x": 150, "y": 10, "width": 128, "height": 32
}];

const var root = dc.setData(data);

dc.setValueCallback(function(id, value)
{
    Console.print(id + " = " + value);
});
```

## Alternatives
- `ScriptPanel` -- Use for static custom-drawn content with manual child management. ScriptDynamicContainer is better for data-driven dynamic component creation.
- `ScriptedViewport` -- Use for scrollable table/list display. ScriptDynamicContainer is better for dynamic component lists from data models.

## Related Preprocessors
None.

## Diagnostic Ideas
Reviewed: Yes
Count: 1
- ScriptDynamicContainer.setValueCallback -- timeline dependency (logged)
