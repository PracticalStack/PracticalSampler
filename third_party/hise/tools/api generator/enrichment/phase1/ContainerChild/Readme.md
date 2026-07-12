# ContainerChild -- Class Analysis

## Brief
Reference handle to a child component inside a ScriptDynamicContainer with property access, hierarchy traversal, and callbacks.

## Purpose
ContainerChild is a scripting handle to a single child component within a ScriptDynamicContainer's data-driven component tree. It provides property get/set with dot-assignment syntax, recursive hierarchy traversal, custom paint routines, value and child-change callbacks, dynamic child manipulation (add/remove), Base64 serialization, and user preset state persistence. Instances are obtained from `ScriptDynamicContainer.setData()` or from other ContainerChild references via `getComponent()`, `getAllComponents()`, `getParent()`, or `addChildComponent()`.

## Details

### Data Model

ContainerChild operates on a two-ValueTree model maintained by `dyncomp::Data`:

- **Data tree** -- Hierarchical component structure with layout/appearance properties. Each ContainerChild wraps one node in this tree.
- **Values tree** -- Flat key-value store where each component's runtime value is stored by its `id`.

This separation means `set()`/`get()` and dot assignment modify the Data tree (appearance/layout), while `setValue()`/`getValue()` operate on the Values tree (interaction state).

### Property System

ContainerChild uses a dedicated property set (not the ScriptComponent property system). Properties are validated against a fixed list -- setting an unknown property throws a script error. Dot assignment (`ref.text = "Hello"`) and `set("text", "Hello")` are equivalent, except that `get()` falls back to default values while dot-read returns the raw property (possibly undefined).

Supported property groups:

| Group | Properties |
|-------|-----------|
| Common | `id`, `type`, `text`, `enabled`, `visible`, `tooltip`, `defaultValue`, `useUndoManager` |
| Position | `x`, `y`, `width`, `height` |
| CSS | `class`, `elementStyle` |
| Button | `isMomentary`, `radioGroupId`, `setValueOnClick` |
| Slider | `min`, `max`, `middlePosition`, `stepSize`, `mode`, `suffix`, `style`, `showValuePopup` |
| ComboBox | `items`, `useCustomPopup`, `popupMenuAlign` |
| Label | `editable`, `multiline`, `updateEachKey` |
| Visual | `filmstripImage`, `numStrips`, `isVertical`, `scaleFactor`, `animationSpeed`, `dragMargin` |
| Connection | `processorId`, `parameterId` |
| Colours | `bgColour`, `itemColour`, `itemColour2`, `textColour` |

### Value Flow

- `setValue(x)` writes to the Values tree but does NOT trigger the control callback. See `setValue()` and `changed()` for details.
- `setValueWithUndo(x)` writes to the Values tree using the global undo manager (regardless of the component's `useUndoManager` setting). See `setValueWithUndo()`.
- The control callback receives one argument: the new value. It deduplicates -- see `setControlCallback()` for registration and callback signature.

### Validity

ContainerChild references become invalid when:
1. The parent container calls `setData()` again (invalidates ALL previous references)
2. The component is removed via `removeFromParent()`
3. The parent ScriptDynamicContainer is destroyed

All mutating methods check validity and throw a script error if the reference is invalid. Once invalid, a reference cannot become valid again. Use `isValid()` to check.

### Undo Manager

If a component's JSON data includes `"useUndoManager": true`, property changes via `set()` and dot assignment, `addChildComponent()`, `removeFromParent()`, and `removeAllChildren()` are undoable. `setValue()` always bypasses undo; use `setValueWithUndo()` for undoable value changes.

### Deferred Operations

`removeFromParent()`, `removeAllChildren()`, and `fromBase64()` are executed asynchronously via `SafeAsyncCall`. This means the component tree is not modified immediately upon return. See individual method entries for timing implications.

### JSON Serialization

`JSON.stringify(childRef)` produces the JSON representation of the component's data properties (not values). This is enabled by the `ObjectWithJSONConverter` interface.

## obtainedVia
`ScriptDynamicContainer.setData(jsonData)` -- returns the root ContainerChild reference.

## minimalObjectToken
cc

## Constants
None.

## Dynamic Constants
None.

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| Storing a ContainerChild reference and using it after calling `setData()` again on the parent | Re-obtain references after each `setData()` call, or check `cc.isValid()` | `setData()` invalidates all previously returned ContainerChild references. Using an invalid reference throws a script error. |
| Expecting `setValue()` to trigger the control callback | Call `cc.changed()` after `cc.setValue(x)` to fire the callback | `setValue()` writes the value silently. `changed()` is needed to trigger the control callback and visual refresh. |
| Reading a property via dot syntax and expecting a default value | Use `cc.get("propertyName")` for default fallback | Dot-read (`cc.text`) returns the raw ValueTree property which may be undefined. `get()` falls back to the property's default value. |

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
}];

const var cc = dc.setData(data);

// Access a child by ID
const var knob = cc.getComponent("Knob1");

knob.setControlCallback(function(value)
{
    Console.print("Knob value: " + value);
});
```

## Alternatives
- `ScriptDynamicContainer` -- The parent container that owns and manages all children. ContainerChild is the per-child reference handle.
- `ScriptPanel` -- Standalone UI component with full paint and mouse callback support. ContainerChild is a lightweight child reference within a dynamic container.

## Related Preprocessors
None.

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: The two callback methods (setControlCallback, setChildCallback) already have ADD_CALLBACK_DIAGNOSTIC registrations in C++, so no additional parse-time diagnostics are needed from the enrichment pipeline.
