<!-- Diagram triage:
  - (no diagrams in Phase 1 data)
-->

# ContainerChild

ContainerChild is a scripting handle to a single child component within a [ScriptDynamicContainer]($API.ScriptDynamicContainer$). It provides access to a data-driven component tree where you can read and set properties, traverse the hierarchy, register value and paint callbacks, and dynamically add or remove children at runtime.

The class operates on two separate data stores:

1. **Data tree** - hierarchical component structure with layout and appearance properties. `set()`, `get()`, and dot assignment (`cc.text = "Hello"`) operate here.
2. **Values tree** - flat key-value store for runtime interaction state. `setValue()`, `getValue()`, and `setValueWithUndo()` operate here.

This separation means changing a property like `text` or `width` does not affect the component's value, and vice versa. `setValue()` does not trigger the control callback on its own - call `changed()` afterward to fire it.

Obtain the root reference from `ScriptDynamicContainer.setData()`, then navigate the tree with `getComponent()`, `getAllComponents()`, or `getParent()`:

```js
const var dc = Content.addDynamicContainer("MyContainer", 0, 0);
const var cc = dc.setData(jsonData);
const var knob = cc.getComponent("Knob1");

// getAllComponents uses wildcard matching (* and ?), NOT regex
const var allSliders = cc.getAllComponents("*Slider*");
```

Use `isEqual()` to compare ContainerChild references - it accepts a string ID or another ContainerChild reference. The `==` operator does not work for reference identity comparison.

> ContainerChild references become invalid when `ScriptDynamicContainer.setData()` is called again, when the component is removed via `removeFromParent()`, or when the parent container is destroyed. Once invalid, a reference cannot be reused. Use `isValid()` to check before operating on a stored reference.

> Three methods execute asynchronously: `removeFromParent()`, `removeAllChildren()`, and `fromBase64()`. The component tree is not updated immediately when these return - subsequent reads may still reflect the old state.

> When a component's JSON data includes `"useUndoManager": true`, property changes via `set()` and dot assignment, `addChildComponent()`, `removeFromParent()`, and `removeAllChildren()` become undoable. `setValue()` always bypasses undo; use `setValueWithUndo()` for undoable value changes.

## Property Access

ContainerChild supports dot-syntax for both reading and writing properties:

```js
// Dot-write (equivalent to cc.set("text", "New Label"))
cc.text = "New Label";
cc.enabled = false;
cc.bgColour = 0xFF333333;

// Dot-read (returns raw ValueTree property - may be undefined)
var label = cc.text;

// Safe read with default fallback
var label = cc.get("text"); // returns "" if not set
```

> [!Warning:Dot-read vs get() for optional properties] Dot-read syntax (`cc.text`) returns the raw ValueTree property, which may be `undefined` if the property was not set in the JSON data. Use `cc.get("propertyName")` to get the default value instead.

## Value Callbacks

There are two levels of value monitoring:

**Container-level** - fires for any child value change. Set on the ScriptDynamicContainer, not on the ContainerChild:

```js
dc.setValueCallback(function(id, value)
{
    // id = component ID string, value = new value
    if (id == "Volume")
        Synth.getEffect("Gain").setAttribute(0, value);
});
```

**Per-child** - fires only for a specific component:

```js
var knob = cc.getComponent("Volume");
knob.setControlCallback(function(value)
{
    // Note: 1 argument (value only), not 2 like ScriptComponent callbacks
    Console.print("Volume: " + value);
});
```

> [!Warning:Callback receives 1 argument, not 2] The per-child `setControlCallback()` receives only the value as its argument. This differs from the standard ScriptComponent control callback which receives (component, value).

The callback deduplicates: setting the same value twice in a row fires the callback only once. Inside the callback, `this` refers to the ContainerChild.

## Child Change Monitoring

Track when children are added or removed from a container-type child:

```js
var container = cc.getComponent("FXList");

container.setChildCallback(function(childId, wasAdded)
{
    if (wasAdded)
        Console.print("Added: " + childId);
    else
        Console.print("Removed: " + childId);
});
```

The callback fires synchronously during the add/remove operation. Inside the callback, `this` refers to the ContainerChild.

## Custom Paint Routines

Children of type `"Panel"` can have custom paint routines. The callback receives a Graphics object. Inside the callback, `this` refers to the ContainerChild:

```js
var panel = cc.getComponent("MyPanel");

panel.setPaintRoutine(function(g)
{
    g.setColour(this.get("bgColour"));
    g.fillRoundedRectangle(this.getLocalBounds(1), 2);
    g.setColour(Colours.white);
    g.drawAlignedText(this.get("text"), this.getLocalBounds(0), "centred");
});
```

The paint routine triggers an immediate repaint on registration - you do not need to call `sendRepaintMessage()` after the initial setup. For subsequent updates, call `sendRepaintMessage()` explicitly.

> [!Warning:this is a ContainerChild, not a ScriptPanel] Inside `setPaintRoutine()`, `this` is a ContainerChild reference. Use `this.get("property")` and `this.getLocalBounds()` instead of ScriptPanel-style `this.data.property`. ScriptPanel callbacks like `setMouseCallback()`, `setTimerCallback()`, and animation features are not available on ContainerChild.

## Dynamic Child Manipulation

Unlike static HISE components, ContainerChild supports runtime addition and removal of components.

### Adding children

```js
var newChild = container.addChildComponent({
    "id": "NewKnob",
    "type": "Slider",
    "text": "Cutoff",
    "min": 20.0,
    "max": 20000.0,
    "mode": "Frequency",
    "width": 128,
    "height": 48
});

// The returned reference is immediately usable
newChild.setControlCallback(function(value)
{
    Console.print("Cutoff: " + value);
});
```

> [!Tip:Children are always appended] `addChildComponent()` always adds the new child at the end. There is no API to insert at a specific index.

### Removing children

```js
// Remove a specific child
var child = container.getComponent("OldKnob");
child.removeFromParent();

// Remove all children
container.removeAllChildren();
```

> [!Warning:Removal is deferred] Both `removeFromParent()` and `removeAllChildren()` execute asynchronously via SafeAsyncCall. The component tree is not updated immediately - `getNumChildComponents()` may still return the old count right after the call. `removeFromParent()` also clears the value callback as a side effect.

### Checking validity

After removal or a new `setData()` call, references become invalid:

```js
if (child.isValid())
{
    // Safe to use
    child.set("text", "Updated");
}
```

`isValid()` has a side effect: when it detects invalidity, it permanently disconnects the reference from the refresh system. This is a one-way transition - once invalid, the reference cannot become valid again.

Use `isEqual()` to compare ContainerChild references. It accepts a string ID or another ContainerChild reference. The `==` operator does not work for reference identity comparison.

## Serialization and User Presets

### Manual serialization with Base64

Save and restore the entire state of a ContainerChild subtree:

```js
// Save state (including runtime values)
var state = cc.toBase64(true);  // true = include values

// Restore state (deferred - not immediate)
cc.fromBase64(state);
```

> [!Warning:fromBase64 is deferred] Restoration via `fromBase64()` is executed asynchronously. Do not read component state immediately after calling it - the properties may still reflect the old state until the async call completes.

### Automatic user preset integration

Register a ContainerChild subtree with the user preset system for automatic save/restore:

```js
var cc = dc.setData(myData);

// Register the root for automatic preset persistence
cc.addStateToUserPreset(true);

// The entire subtree (properties + values) will now be saved/restored
// with user presets automatically via Base64 serialization.

// To unregister:
cc.addStateToUserPreset(false);
```

This uses the same `toBase64(true)` / `fromBase64()` mechanism internally but handles the save/restore lifecycle automatically through the `UserPresetHandler`.

## Common Mistakes

- **Re-obtain references after setData()**
  **Wrong:** Storing a ContainerChild reference and using it after calling `setData()` again on the parent
  **Right:** Re-obtain references after each `setData()` call, or check with `isValid()` before use
  *`setData()` invalidates all previously returned ContainerChild references. Using an invalid reference throws a script error.*

- **Call changed() after setValue()**
  **Wrong:** `cc.setValue(0.5);` and expecting the control callback to fire
  **Right:** `cc.setValue(0.5); cc.changed();`
  *`setValue()` writes the value silently. `changed()` is needed to trigger the control callback and visual refresh.*

- **Use get() for default fallback**
  **Wrong:** Reading a property via dot syntax (`cc.text`) and expecting a default value when unset
  **Right:** Use `cc.get("text")` which falls back to the property's default value
  *Dot-read returns the raw property which may be undefined if not explicitly set. `get()` provides the default.*
