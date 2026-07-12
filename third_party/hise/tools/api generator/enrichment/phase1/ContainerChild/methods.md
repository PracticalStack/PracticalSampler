## addChildComponent

**Signature:** `var addChildComponent(JSON childData)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Minimal Example:** `var child = {obj}.addChildComponent({"id": "NewBtn", "type": "Button", "text": "Click"});`

**Description:**
Creates a new child component from the given JSON data and appends it as the last child of this component. Returns a ContainerChild reference to the new child. Position can be specified as a `bounds` array `[x, y, w, h]` or as individual `x`, `y`, `width`, `height` properties (defaults: `0, 0, 128, 50`). Respects the undo manager if `useUndoManager` is true on this component.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| childData | JSON | no | Component definition object | Must contain valid dyncomp properties |

**Pitfalls:**
- Children are always appended at the end. There is no API to insert at a specific index.

**Cross References:**
- `$API.ContainerChild.removeFromParent$`
- `$API.ContainerChild.getComponent$`

**Example:**
```javascript:add-child-both-formats
// Title: Adding child components with bounds array and individual properties
const var dc = Content.addDynamicContainer("DC1", 0, 0);
const var cc = dc.setData({"id": "Root", "type": "Container", "width": 500, "height": 300});

// Using bounds array
var slider = cc.addChildComponent({
    "id": "Vol",
    "type": "Slider",
    "bounds": [10, 10, 128, 48]
});

// Using individual position properties (defaults: 0, 0, 128, 50)
var btn = cc.addChildComponent({
    "id": "Mute",
    "type": "Button",
    "text": "Mute",
    "x": 150, "y": 10, "width": 80, "height": 32
});

Console.print(cc.getNumChildComponents()); // 2
```
```json:testMetadata:add-child-both-formats
{
  "testable": true,
  "verifyScript": [
    {"type": "log-output", "values": ["2"]},
    {"type": "REPL", "expression": "slider.get(\"type\")", "value": "Slider"},
    {"type": "REPL", "expression": "btn.get(\"text\")", "value": "Mute"}
  ]
}
```

## addStateToUserPreset

**Signature:** `void addStateToUserPreset(Integer shouldAdd)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.addStateToUserPreset(true);`

**Description:**
Registers or unregisters this ContainerChild with the UserPresetHandler as a state manager. When registered, the component's entire subtree (data properties, children, and values) is automatically saved and restored with user presets via Base64 serialization. Pass `false` to unregister.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shouldAdd | Integer | no | true to register, false to unregister | Boolean |

**Cross References:**
- `$API.ContainerChild.toBase64$`
- `$API.ContainerChild.fromBase64$`

## changed

**Signature:** `void changed()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.changed();`

**Description:**
Triggers the control callback (if registered via `setControlCallback`) and sends a visual refresh message to the component. Call this after `setValue()` to fire the callback. Unlike ScriptComponent's `changed()`, this works during `onInit` because it uses synchronous ValueTree listeners internally.

**Parameters:**

None.

**Cross References:**
- `$API.ContainerChild.setValue$`
- `$API.ContainerChild.setControlCallback$`

## fromBase64

**Signature:** `void fromBase64(String b64)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.fromBase64(savedState);`

**Description:**
Restores this component's properties, children, and optionally values from a Base64-encoded string produced by `toBase64()`. The restoration is deferred via `SafeAsyncCall` -- the component tree is not updated immediately upon return. Uses the undo manager if `useUndoManager` is true.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| b64 | String | no | Base64-encoded state string from toBase64() | Must be a valid zstd-compressed Base64 string |

**Pitfalls:**
- Restoration is deferred -- the component tree is not updated immediately after this call returns. Subsequent reads may still reflect the old state.

**Cross References:**
- `$API.ContainerChild.toBase64$`

## get

**Signature:** `var get(String id)`
**Return Type:** `NotUndefined`
**Call Scope:** unsafe
**Minimal Example:** `var text = {obj}.get("text");`

**Description:**
Returns the value of the specified property. If the property has not been explicitly set on this component, returns the default value for that property. Throws a script error if the property name is not in the valid property list. Unlike dot-read syntax (`cc.text`), this method provides default value fallback.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| id | String | no | Property name | Must be a valid dyncomp property name |

**Cross References:**
- `$API.ContainerChild.set$`

## getAllComponents

**Signature:** `Array getAllComponents(String regex)`
**Return Type:** `Array`
**Call Scope:** unsafe
**Minimal Example:** `var buttons = {obj}.getAllComponents("Btn*");`

**Description:**
Recursively searches all descendant components and returns an array of ContainerChild references whose `id` matches the given wildcard pattern. Despite the parameter name, this uses wildcard matching (with `*` and `?` globs), not regular expressions. Returns an empty array if no matches are found.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| regex | String | no | Wildcard pattern to match component IDs | Uses `*` and `?` glob syntax, not regex |

**Pitfalls:**
- The parameter is named `regex` but uses wildcard/glob matching, not regular expressions.

**Cross References:**
- `$API.ContainerChild.getComponent$`

## getChildComponentIndex

**Signature:** `int getChildComponentIndex(NotUndefined childIdOrComponent)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Minimal Example:** `var idx = {obj}.getChildComponentIndex("Knob1");`

**Description:**
Returns the index of the specified child among this component's direct children. Accepts either a string ID or a ContainerChild reference. Returns -1 if the child is not found. Only searches direct children -- not recursive.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| childIdOrComponent | NotUndefined | no | Child component ID string or ContainerChild reference | String or ScriptObject (ContainerChild) |

**Cross References:**
- `$API.ContainerChild.getComponent$`
- `$API.ContainerChild.getNumChildComponents$`

## getComponent

**Signature:** `var getComponent(String childId)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Minimal Example:** `var child = {obj}.getComponent("Knob1");`

**Description:**
Recursively searches all descendant components and returns a ContainerChild reference to the first component with the matching `id`. Returns `undefined` if no match is found -- does not throw an error.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| childId | String | no | Component ID to search for | Exact string match |

**Cross References:**
- `$API.ContainerChild.getAllComponents$`
- `$API.ContainerChild.getChildComponentIndex$`

## getLocalBounds

**Signature:** `Array getLocalBounds(Integer margin)`
**Return Type:** `Array`
**Call Scope:** unsafe
**Minimal Example:** `var bounds = {obj}.getLocalBounds(0);`

**Description:**
Returns the component's bounds as an array `[x, y, width, height]` at the origin (x=0, y=0), reduced by the given margin on all sides. Uses the component's `width` and `height` properties (defaults: 128, 50 if not set).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| margin | Integer | no | Pixel margin to subtract from all sides | Non-negative |

**Cross References:**
- `$API.ContainerChild.setBounds$`

## getNumChildComponents

**Signature:** `int getNumChildComponents()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var count = {obj}.getNumChildComponents();`

**Description:**
Returns the number of direct child components. Does not count descendants recursively. Does not check validity -- may return a stale count on an invalid reference.

**Parameters:**

None.

**Cross References:**
- `$API.ContainerChild.getChildComponentIndex$`

## getParent

**Signature:** `var getParent()`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Minimal Example:** `var parent = {obj}.getParent();`

**Description:**
Returns a ContainerChild reference to this component's parent in the data tree. On the root component returned by `setData()`, this returns a reference to the data tree root node itself.

**Parameters:**

None.

**Cross References:**
- `$API.ContainerChild.getComponent$`
- `$API.ContainerChild.addChildComponent$`

## getValue

**Signature:** `var getValue()`
**Return Type:** `NotUndefined`
**Call Scope:** warning
**Call Scope Note:** String involvement: constructs component ID string for Values tree lookup.
**Minimal Example:** `var val = {obj}.getValue();`

**Description:**
Returns this component's current value from the Values tree. If no value has been set, returns the `defaultValue` property from the component's data. Does not check validity -- can return stale data from an invalid reference without throwing an error.

**Parameters:**

None.

**Pitfalls:**
- [BUG] Does not call `isValidOrThrow()` unlike the sibling `get()` method. On an invalid reference, this returns stale data without warning instead of throwing a script error.

**Cross References:**
- `$API.ContainerChild.setValue$`
- `$API.ContainerChild.changed$`

## isEqual

**Signature:** `bool isEqual(NotUndefined other)`
**Return Type:** `Integer`
**Call Scope:** warning
**Call Scope Note:** String involvement when comparing by ID.
**Minimal Example:** `var same = {obj}.isEqual("Knob1");`

**Description:**
Checks whether this component matches the given argument. Accepts either a string (compared against the component's `id` property) or a ContainerChild reference (compared by ValueTree identity). Returns `false` if the argument type is neither string nor ContainerChild.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| other | NotUndefined | no | Component ID string or ContainerChild reference | String or ScriptObject (ContainerChild) |

## isValid

**Signature:** `bool isValid()`
**Return Type:** `Integer`
**Call Scope:** warning
**Call Scope Note:** May modify internal invalidation state as a side effect.
**Minimal Example:** `var ok = {obj}.isValid();`

**Description:**
Checks whether this reference still points to a valid component within the container's data tree. Returns `false` if the reference was invalidated (e.g., by `setData()` or `removeFromParent()`). Once invalidity is detected, the reference permanently disconnects from the refresh broadcaster -- this is a one-way transition.

**Parameters:**

None.

**Pitfalls:**
- Has a side effect: when invalidity is detected, it disconnects the refresh broadcaster listener and sets the invalid flag permanently. This is a one-time cleanup, not a pure query.

**Cross References:**
- `$API.ContainerChild.removeFromParent$`
- `$API.ScriptDynamicContainer.setData$`

## loseFocus

**Signature:** `void loseFocus(Integer recursive)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.loseFocus(true);`

**Description:**
Sends a message to the component to release keyboard focus. When `recursive` is true, the message propagates to all descendant components.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| recursive | Integer | no | Whether to propagate to descendants | Boolean |

## removeAllChildren

**Signature:** `void removeAllChildren()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.removeAllChildren();`

**Description:**
Removes all child components from this component. Execution is deferred via `SafeAsyncCall` -- the children are not removed immediately upon return. Uses this component's undo manager if `useUndoManager` is true.

**Parameters:**

None.

**Pitfalls:**
- Removal is deferred -- `getNumChildComponents()` may still return the old count immediately after this call.

**Cross References:**
- `$API.ContainerChild.removeFromParent$`
- `$API.ContainerChild.addChildComponent$`

## removeFromParent

**Signature:** `void removeFromParent()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.removeFromParent();`

**Description:**
Removes this component from its parent. Execution is deferred via `SafeAsyncCall`. Before removal, clears the value callback and recursively removes all descendant values from the Values tree. Uses the parent's undo manager (not this component's) if the parent has `useUndoManager` set to true.

**Parameters:**

None.

**Pitfalls:**
- Removal is deferred -- the component is still in the tree immediately after this call returns.
- Clears the value callback as a side effect. After removal, the control callback will not fire even if the reference is somehow still accessible.
- Uses the parent's undo manager for the removal, not this component's. Undo behavior depends on the parent's `useUndoManager` property.

**Cross References:**
- `$API.ContainerChild.removeAllChildren$`
- `$API.ContainerChild.isValid$`

## resetValueToDefault

**Signature:** `void resetValueToDefault(Integer recursive)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.resetValueToDefault(false);`

**Description:**
Sends a message to the component to reset its value to the `defaultValue` property. When `recursive` is true, the message propagates to all descendant components.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| recursive | Integer | no | Whether to propagate to descendants | Boolean |

**Cross References:**
- `$API.ContainerChild.getValue$`

## sendRepaintMessage

**Signature:** `void sendRepaintMessage(Integer recursive)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.sendRepaintMessage(false);`

**Description:**
Sends a repaint message to this component. If a paint routine is registered, it will be re-executed on the JavaScript thread pool. When `recursive` is true, the message propagates to all descendant components.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| recursive | Integer | no | Whether to propagate to descendants | Boolean |

**Cross References:**
- `$API.ContainerChild.setPaintRoutine$`

## set

**Signature:** `void set(String id, NotUndefined newValue)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.set("text", "Volume");`

**Description:**
Sets a component property on the Data tree. Throws a script error if the property name is not in the valid property list. Equivalent to dot-assignment syntax (`cc.text = "Volume"`). Respects the undo manager if `useUndoManager` is true.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| id | String | no | Property name | Must be a valid dyncomp property name |
| newValue | NotUndefined | no | New property value | Type depends on the property |

**Cross References:**
- `$API.ContainerChild.get$`

## setBounds

**Signature:** `void setBounds(Array area)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setBounds([10, 20, 200, 100]);`

**Description:**
Sets the component's position and size from a rectangle array `[x, y, width, height]`. Equivalent to setting `x`, `y`, `width`, and `height` properties individually but in a single call. Respects the undo manager.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| area | Array | no | Rectangle as [x, y, width, height] | Must be a valid rectangle array |

**Cross References:**
- `$API.ContainerChild.getLocalBounds$`

## setChildCallback

**Signature:** `void setChildCallback(Function newChildCallback)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setChildCallback(onChildChange);`

**Description:**
Registers a callback that fires whenever a direct child component is added to or removed from this component. The callback fires synchronously during the add/remove operation. Inside the callback, `this` refers to the ContainerChild.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newChildCallback | Function | yes | Callback function | Must accept 2 arguments |

**Callback Signature:** newChildCallback(childId: String, wasAdded: bool)

**Cross References:**
- `$API.ContainerChild.addChildComponent$`
- `$API.ContainerChild.removeFromParent$`
- `$API.ContainerChild.removeAllChildren$`

**Example:**
```javascript:child-callback-basic
// Title: Monitoring child add/remove events
const var dc = Content.addDynamicContainer("DC2", 0, 0);
const var cc = dc.setData({"id": "Parent", "type": "Container"});

reg childLog = "";

inline function onChildChange(childId, wasAdded)
{
    childLog = childId + ":" + wasAdded;
}

cc.setChildCallback(onChildChange);
cc.addChildComponent({"id": "NewChild", "type": "Button"});
// childLog is updated asynchronously after onInit
```
```json:testMetadata:child-callback-basic
{
  "testable": true,
  "verifyScript": {"type": "REPL", "expression": "childLog", "value": "NewChild:1", "delay": 300}
}
```

## setControlCallback

**Signature:** `void setControlCallback(Function controlCallback)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setControlCallback(onValueChanged);`

**Description:**
Registers a callback that fires when this component's value changes. The callback receives one argument (the new value) and deduplicates -- it only fires when the value actually differs from the previous value. The callback fires synchronously via ValueTree property listeners, so it works during `onInit` (unlike ScriptComponent's `changed()`). Inside the callback, `this` refers to the ContainerChild. Note: `setValue()` alone does NOT trigger this callback -- call `changed()` afterward.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| controlCallback | Function | yes | Value change callback | Must accept 1 argument |

**Callback Signature:** controlCallback(value: var)

**Pitfalls:**
- The callback receives 1 argument (the value), not 2 like ScriptComponent's control callback (component, value). This is a common source of confusion.
- The callback deduplicates: setting the same value twice in a row only fires the callback once.

**Cross References:**
- `$API.ContainerChild.setValue$`
- `$API.ContainerChild.changed$`

**Example:**
```javascript:control-callback-basic
// Title: Value callback with deduplication
const var dc = Content.addDynamicContainer("DC3", 0, 0);
const var cc = dc.setData({"id": "Knob1", "type": "Slider", "defaultValue": 0.0});

reg lastVal = -1.0;

inline function onKnobValue(value)
{
    lastVal = value;
}

cc.setControlCallback(onKnobValue);
cc.setValue(0.75);
cc.changed();
```
```json:testMetadata:control-callback-basic
{
  "testable": true,
  "verifyScript": {"type": "REPL", "expression": "lastVal", "value": 0.75}
}
```

## setPaintRoutine

**Signature:** `void setPaintRoutine(Function newPaintRoutine)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setPaintRoutine(onPaint);`

**Description:**
Registers a paint callback that draws this component's visual content. The callback receives a Graphics object as its single argument. Inside the callback, `this` refers to the ContainerChild. Paint execution is dispatched to the JavaScript thread pool as a low-priority task. An immediate repaint is triggered upon registration.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newPaintRoutine | Function | no | Paint callback | Must accept 1 argument (Graphics) |

**Callback Signature:** newPaintRoutine(g: Graphics)

**Pitfalls:**
- [BUG] Uses `isValid()` instead of `isValidOrThrow()`, unlike `setControlCallback()` and `setChildCallback()`. On an invalid reference, the call silently does nothing instead of throwing a script error.
- Triggers an immediate repaint on registration. The paint routine runs once automatically without needing a `sendRepaintMessage()` call.

**Cross References:**
- `$API.ContainerChild.sendRepaintMessage$`
- `$API.ContainerChild.getLocalBounds$`

**Example:**
```javascript:paint-routine-basic
// Title: Custom paint routine for a container child
const var dc = Content.addDynamicContainer("DC4", 0, 0);
dc.setPosition(0, 0, 500, 300);
const var cc = dc.setData({"id": "Panel1", "type": "Container", "width": 200, "height": 100});

inline function onPaint(g)
{
    local b = this.getLocalBounds(0);
    g.setColour(Colours.darkgrey);
    g.fillRect(b);
    g.setColour(Colours.white);
    g.drawAlignedText(this.get("text"), b, "centred");
}

cc.set("text", "Hello");
cc.setPaintRoutine(onPaint);
```
```json:testMetadata:paint-routine-basic
{
  "testable": false,
  "skipReason": "Paint routine executes asynchronously on the JavaScript thread pool; cannot verify visual output via REPL"
}
```

## setValue

**Signature:** `void setValue(NotUndefined newValue)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setValue(0.5);`

**Description:**
Sets this component's value in the Values tree without triggering the control callback. The undo manager is explicitly bypassed (always `nullptr`). To trigger the control callback after setting the value, call `changed()`. For undoable value changes, use `setValueWithUndo()`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newValue | NotUndefined | no | New value | Any type |

**Cross References:**
- `$API.ContainerChild.changed$`
- `$API.ContainerChild.setValueWithUndo$`
- `$API.ContainerChild.getValue$`

## setValueWithUndo

**Signature:** `void setValueWithUndo(NotUndefined newValue)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setValueWithUndo(0.5);`

**Description:**
Sets this component's value in the Values tree using the global undo manager, without triggering the control callback. Unlike `setValue()`, this always uses the global undo manager regardless of the component's `useUndoManager` property. To trigger the control callback afterward, call `changed()`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newValue | NotUndefined | no | New value | Any type |

**Pitfalls:**
- Always uses the global undo manager, not the component's configured undo manager. This means undo works even if `useUndoManager` is false on the component.

**Cross References:**
- `$API.ContainerChild.setValue$`
- `$API.ContainerChild.changed$`

## toBase64

**Signature:** `String toBase64(Integer includeValues)`
**Return Type:** `String`
**Call Scope:** unsafe
**Minimal Example:** `var state = {obj}.toBase64(true);`

**Description:**
Serializes this component's data properties and children into a Base64-encoded, zstd-compressed string. When `includeValues` is true, the current runtime values of this component and all descendants are also included. The resulting string can be passed to `fromBase64()` to restore the state.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| includeValues | Integer | no | Whether to include runtime values | Boolean |

**Cross References:**
- `$API.ContainerChild.fromBase64$`
- `$API.ContainerChild.addStateToUserPreset$`

## updateValueFromProcessorConnection

**Signature:** `void updateValueFromProcessorConnection(Integer recursive)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.updateValueFromProcessorConnection(false);`

**Description:**
Sends a message to refresh this component's value from its connected processor parameter (set via `processorId` and `parameterId` properties). When `recursive` is true, the message propagates to all descendant components.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| recursive | Integer | no | Whether to propagate to descendants | Boolean |

**Cross References:**
- `$API.ContainerChild.set$`
