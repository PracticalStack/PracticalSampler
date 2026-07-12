# ScriptDynamicContainer -- Methods

## addToMacroControl

**Disabled:** property-deactivated
**Disabled Reason:** The `macroControl` property is deactivated on ScriptDynamicContainer. The container does not participate in the macro control system.

---

## changed

**Signature:** `undefined changed()`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.changed();`

**Description:**
Triggers the control callback (either the custom one set via `setControlCallback` or the default `onControl` callback) for the container's own value. Also notifies any registered value listeners. This triggers the container-level callback, not the dyncomp child value callbacks registered via `setValueCallback`.

**Pitfalls:**
- Cannot be called during `onInit` -- if called during `onInit`, it logs a console message and returns without executing.

**Cross References:**
- `$API.ScriptDynamicContainer.setControlCallback$`
- `$API.ScriptDynamicContainer.getValue$`
- `$API.ScriptDynamicContainer.setValueCallback$`

---

## fadeComponent

**Signature:** `undefined fadeComponent(Integer shouldBeVisible, Integer milliseconds)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.fadeComponent(1, 500);`

**Description:**
Toggles visibility with a fade animation over the specified duration in milliseconds. Only triggers if the target visibility differs from the current visibility. Sets the `visible` property and sends an async fade message through the global UI animator.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shouldBeVisible | Integer | no | Target visibility state | 1 = show, 0 = hide |
| milliseconds | Integer | no | Duration of the fade animation in milliseconds | > 0 |

**Cross References:**
- `$API.ScriptDynamicContainer.showControl$`

---

## get

**Signature:** `var get(String propertyName)`
**Return Type:** `var`
**Call Scope:** safe
**Minimal Example:** `var v = {obj}.get("visible");`

**Description:**
Returns the current value of the named property. Reports a script error if the property does not exist. Active properties on ScriptDynamicContainer: `visible`, `enabled`, `locked`, `x`, `y`, `width`, `height`, `bgColour`, `itemColour`, `itemColour2`, `textColour`, `useUndoManager`, `parentComponent`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| propertyName | String | no | The name of a component property to retrieve | Must be a valid active property ID for this component type |

**Cross References:**
- `$API.ScriptDynamicContainer.set$`
- `$API.ScriptDynamicContainer.getAllProperties$`

---

## getAllProperties

**Signature:** `Array getAllProperties()`
**Return Type:** `Array`
**Call Scope:** unsafe
**Minimal Example:** `var props = {obj}.getAllProperties();`

**Description:**
Returns an array of strings containing all active (non-deactivated) property IDs for this component. ScriptDynamicContainer deactivates 16 base properties, so this returns a reduced set: `visible`, `enabled`, `locked`, `x`, `y`, `width`, `height`, `bgColour`, `itemColour`, `itemColour2`, `textColour`, `useUndoManager`, `parentComponent`.

**Cross References:**
- `$API.ScriptDynamicContainer.get$`
- `$API.ScriptDynamicContainer.set$`

---

## getChildComponents

**Signature:** `Array getChildComponents()`
**Return Type:** `Array`
**Call Scope:** unsafe
**Minimal Example:** `var children = {obj}.getChildComponents();`

**Description:**
Returns an array of ScriptComponent references for all child components whose `parentComponent` property is set to this container. This returns regular ScriptComponent children, not dyncomp children created via `setData()`. DynComp children are managed through the ContainerChild reference system returned by `setData()`.

---

## getGlobalPositionX

**Signature:** `Integer getGlobalPositionX()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var x = {obj}.getGlobalPositionX();`

**Description:**
Returns the absolute x-position relative to the interface root, computed by recursively adding parent component x-offsets.

**Cross References:**
- `$API.ScriptDynamicContainer.getGlobalPositionY$`

---

## getGlobalPositionY

**Signature:** `Integer getGlobalPositionY()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var y = {obj}.getGlobalPositionY();`

**Description:**
Returns the absolute y-position relative to the interface root, computed by recursively adding parent component y-offsets.

**Cross References:**
- `$API.ScriptDynamicContainer.getGlobalPositionX$`

---

## getHeight

**Signature:** `Integer getHeight()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var h = {obj}.getHeight();`

**Description:**
Returns the `height` property as an integer.

---

## getId

**Signature:** `String getId()`
**Return Type:** `String`
**Call Scope:** safe
**Minimal Example:** `var id = {obj}.getId();`

**Description:**
Returns the component's ID as a string (the variable name used when creating the component, e.g. "MyContainer").

---

## getLocalBounds

**Signature:** `Array getLocalBounds(Double reduceAmount)`
**Return Type:** `Array`
**Call Scope:** safe
**Minimal Example:** `var bounds = {obj}.getLocalBounds(0);`

**Description:**
Returns an array `[x, y, w, h]` representing the local bounds reduced by the given amount. The local bounds start at `[0, 0, width, height]`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| reduceAmount | Double | no | The amount in pixels to inset from each edge | >= 0.0 |

---

## getValue

**Signature:** `var getValue()`
**Return Type:** `var`
**Call Scope:** safe
**Minimal Example:** `var v = {obj}.getValue();`

**Description:**
Returns the current value of the container component itself. This returns the container's own ScriptComponent value, not the values of dyncomp children. Use ContainerChild's `getValue()` or the `setValueCallback` listener for child component values.

**Cross References:**
- `$API.ScriptDynamicContainer.setValue$`
- `$API.ScriptDynamicContainer.changed$`
- `$API.ScriptDynamicContainer.setValueCallback$`

---

## getValueNormalized

**Disabled:** redundant
**Disabled Reason:** Base implementation returns `getValue()` directly with no normalization. Only meaningful on ScriptSlider, which maps between actual range and 0..1.

---

## getWidth

**Signature:** `Integer getWidth()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var w = {obj}.getWidth();`

**Description:**
Returns the `width` property as an integer.

---

## grabFocus

**Signature:** `undefined grabFocus()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.grabFocus();`

**Description:**
Notifies z-level listeners that the component wants to grab keyboard focus. Only notifies the first listener (exclusive operation).

**Cross References:**
- `$API.ScriptDynamicContainer.loseFocus$`

---

## loseFocus

**Signature:** `undefined loseFocus()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.loseFocus();`

**Description:**
Notifies all z-level listeners that the component wants to lose keyboard focus.

**Cross References:**
- `$API.ScriptDynamicContainer.grabFocus$`

---

## sendRepaintMessage

**Signature:** `undefined sendRepaintMessage()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.sendRepaintMessage();`

**Description:**
Sends a repaint refresh message through the dyncomp data model's refresh broadcaster, triggering a visual update of all dynamic child components. This overrides the base ScriptComponent implementation, which uses the standard `repaintBroadcaster`. The dyncomp override ensures the refresh propagates through the dynamic component tree rather than just the container's own JUCE component.

**Cross References:**
- `$API.ScriptDynamicContainer.setData$`

---

## set

**Signature:** `undefined set(String propertyName, NotUndefined value)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.set("visible", false);`

**Description:**
Sets a component property to the given value. Reports a script error if the property does not exist. Active properties on ScriptDynamicContainer: `visible`, `enabled`, `locked`, `x`, `y`, `width`, `height`, `bgColour`, `itemColour`, `itemColour2`, `textColour`, `useUndoManager`, `parentComponent`. During `onInit`, changes are applied without UI notification; outside `onInit`, sends change notifications to update the UI.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| propertyName | String | no | The property identifier to set | Must be a valid active property ID for this component type |
| value | NotUndefined | no | The new value for the property | Type must match the property's expected type |

**Cross References:**
- `$API.ScriptDynamicContainer.get$`
- `$API.ScriptDynamicContainer.getAllProperties$`

---

## setConsumedKeyPresses

**Signature:** `undefined setConsumedKeyPresses(NotUndefined listOfKeys)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setConsumedKeyPresses("all");`

**Description:**
Defines which key presses this component consumes. Must be called before `setKeyPressCallback`. Accepts a string, object, or array of either.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| listOfKeys | NotUndefined | no | Key descriptions to consume -- a string, object, or array | See value descriptions below |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| "all" | Catch all key presses exclusively (prevents parent from receiving them) |
| "all_nonexclusive" | Catch all key presses non-exclusively (parent still receives them) |

**Cross References:**
- `$API.ScriptDynamicContainer.setKeyPressCallback$`

---

## setControlCallback

**Signature:** `undefined setControlCallback(Function controlFunction)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setControlCallback(onContainerChanged);`

**Description:**
Assigns a custom inline function as the control callback for the container's own value, replacing the default `onControl` handler. This handles the container's own ScriptComponent value changes, not dyncomp child value changes. Use `setValueCallback` for child component value changes. Pass `undefined` to revert to the default `onControl` callback.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| controlFunction | Function | no | An inline function with 2 parameters (component, value), or undefined to clear | Must be an inline function with exactly 2 parameters |

**Callback Signature:** controlFunction(component: ScriptComponent, value: var)

**Pitfalls:**
- The function MUST be declared with `inline function`. Regular function references are rejected with a script error.
- Must have exactly 2 parameters. Reports a script error if the parameter count is wrong.

**Cross References:**
- `$API.ScriptDynamicContainer.changed$`
- `$API.ScriptDynamicContainer.setValueCallback$`

---

## setData

**Signature:** `ScriptObject setData(var newData)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Creates dyncomp::Data objects with ValueTree allocation, invalidates previous ChildReferences, sends async UI rebuild notification via dataBroadcaster.
**Minimal Example:** `var root = {obj}.setData([{"id": "Knob1", "type": "Slider"}]);`

**Description:**
Creates a dynamic component tree from JSON data. Accepts either a single JSON object (for one component) or an array of JSON objects (for multiple components). Returns a `ContainerChild` reference -- to the single child if one object was passed, or to the root of the tree if an array was passed. All previously returned ContainerChild references are invalidated.

Supported component types: `"Button"`, `"Slider"`, `"ComboBox"`, `"Label"`, `"Panel"`, `"FloatingTile"`, `"DragContainer"`, `"Viewport"`, `"TextBox"`, `"TableEditor"`, `"SliderPack"`, `"AudioFile"`. Legacy names (`ScriptButton`, `ScriptSlider`, `ScriptComboBox`, `ScriptLabel`, `ScriptPanel`, `ScriptViewport`) are accepted and auto-converted.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newData | var | no | A JSON object or array of JSON objects describing the component tree | Each object should have `id` and `type` properties at minimum |

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| id | String | Component identifier (required for value tracking) |
| type | String | Component type (see supported types above) |
| text | String | Display text |
| enabled | bool | Enabled state (default: true) |
| visible | bool | Visibility (default: true) |
| tooltip | String | Tooltip text |
| defaultValue | Double | Default value (default: 0.0) |
| useUndoManager | bool | Enable undo support (default: false) |
| x | int | X position (default: 0) |
| y | int | Y position (default: 0) |
| width | int | Width (default: 128) |
| height | int | Height (default: 50) |
| class | String | CSS class selector |
| elementStyle | String | Inline CSS style |
| bgColour | int | Background colour (default: 0x80000000) |
| itemColour | int | Item colour (default: 0x33FFFFFF) |
| itemColour2 | int | Item colour 2 (default: 0x33FFFFFF) |
| textColour | int | Text colour (default: 0xCCFFFFFF) |
| processorId | String | Processor connection ID |
| parameterId | String | Parameter connection ID |
| filmstripImage | String | Filmstrip image reference |
| numStrips | int | Filmstrip frame count (default: 64) |
| min | Double | Slider minimum (default: 0.0) |
| max | Double | Slider maximum (default: 1.0) |
| middlePosition | Double | Slider skew center (default: -10) |
| stepSize | Double | Slider step size (default: 0.01) |
| mode | String | Slider value mode |
| suffix | String | Slider value suffix |
| style | String | Slider visual style (default: "Knob") |
| items | String | ComboBox item list |
| isMomentary | bool | Button momentary mode (default: false) |
| radioGroupId | int | Button radio group (default: 0) |
| editable | bool | Label editable (default: true) |
| multiline | bool | Label multiline (default: false) |

**Pitfalls:**
- All previously returned ContainerChild references become invalid after calling `setData()`. Using an invalid reference throws a script error.

**Cross References:**
- `$API.ScriptDynamicContainer.setValueCallback$`

**Example:**
```javascript:dyncomp-setup
// Title: Create dynamic controls from JSON data
const var dc = Content.addDynamicContainer("FXControls", 0, 0);
dc.setPosition(0, 0, 400, 200);

const var root = dc.setData([
{
    "id": "GainKnob",
    "type": "Slider",
    "text": "Gain",
    "min": -100.0,
    "max": 0.0,
    "mode": "Decibel",
    "x": 10, "y": 10, "width": 128, "height": 48
},
{
    "id": "BypassBtn",
    "type": "Button",
    "text": "Bypass",
    "x": 150, "y": 10, "width": 100, "height": 32
}]);

Console.print(dc.getWidth());
```
```json:testMetadata:dyncomp-setup
{
  "testable": true,
  "verifyScript": [
    {"type": "log-output", "values": ["400"]},
    {"type": "REPL", "expression": "dc.getWidth()", "value": 400}
  ]
}
```

---

## setKeyPressCallback

**Signature:** `undefined setKeyPressCallback(Function keyboardFunction)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setKeyPressCallback(onKeyPress);`

**Description:**
Registers a callback that fires when a consumed key is pressed while this container has focus. MUST call `setConsumedKeyPresses()` BEFORE calling this method. Reports a script error if `setConsumedKeyPresses` has not been called yet.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| keyboardFunction | Function | no | An inline function with 1 parameter (event object) | Must be an inline function |

**Callback Signature:** keyboardFunction(event: Object)

**Callback Properties:**

Key press event:

| Property | Type | Description |
|----------|------|-------------|
| isFocusChange | bool | Always false for key events |
| character | String | The printable character, or "" for non-printable keys |
| specialKey | bool | true if not a printable character |
| keyCode | int | The JUCE key code |
| description | String | Human-readable description of the key press |
| shift | bool | true if Shift is held |
| cmd | bool | true if Cmd/Ctrl is held |
| alt | bool | true if Alt is held |

Focus change event:

| Property | Type | Description |
|----------|------|-------------|
| isFocusChange | bool | Always true for focus events |
| hasFocus | bool | true if focus gained, false if lost |

**Pitfalls:**
- MUST call `setConsumedKeyPresses()` BEFORE calling this method. Reports a script error otherwise.

**Cross References:**
- `$API.ScriptDynamicContainer.setConsumedKeyPresses$`

---

## setLocalLookAndFeel

**Signature:** `undefined setLocalLookAndFeel(ScriptObject lafObject)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setLocalLookAndFeel(laf);`

**Description:**
Attaches a scripted look and feel object to this container and propagates it to all regular ScriptComponent children. Pass `undefined` to clear it. The propagation only affects ScriptComponent children (those with `parentComponent` set to this container), not dyncomp children created via `setData()`. DynComp children use their own CSS styling via the `class` and `elementStyle` properties.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| lafObject | ScriptObject | no | A ScriptedLookAndFeel object, or undefined to clear | Must be a ScriptedLookAndFeel instance |

**Pitfalls:**
- Propagates to ScriptComponent children automatically but does NOT affect dyncomp children created via `setData()`.
- If the LAF uses CSS (has a stylesheet), automatically calls `setStyleSheetClass({})` to initialize the class selector.

**Cross References:**
- `$API.ScriptDynamicContainer.setStyleSheetClass$`
- `$API.ScriptDynamicContainer.setStyleSheetProperty$`

---

## setPosition

**Signature:** `undefined setPosition(Integer x, Integer y, Integer w, Integer h)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setPosition(10, 10, 400, 300);`

**Description:**
Sets the component's position and size in one call. Directly sets the `x`, `y`, `width`, `height` properties on the property tree.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| x | Integer | no | X position in pixels, relative to parent | 0-900 |
| y | Integer | no | Y position in pixels, relative to parent | 0-MAX_SCRIPT_HEIGHT |
| w | Integer | no | Width in pixels | 0-900 |
| h | Integer | no | Height in pixels | 0-MAX_SCRIPT_HEIGHT |

---

## setPropertiesFromJSON

**Disabled:** no-op
**Disabled Reason:** Not registered as a direct API method on component instances. Exposed on the Content object as `Content.setPropertiesFromJSON(componentName, jsonData)` instead.

---

## setStyleSheetClass

**Signature:** `undefined setStyleSheetClass(String classIds)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setStyleSheetClass(".active");`

**Description:**
Sets the CSS class selectors for this component. The component's own type class (`.scriptdynamiccontainer`) is automatically prepended. Creates the `ComponentStyleSheetProperties` value tree if it does not yet exist.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| classIds | String | no | Space-separated CSS class selectors to apply | e.g. ".myClass .highlighted" |

**Cross References:**
- `$API.ScriptDynamicContainer.setStyleSheetProperty$`
- `$API.ScriptDynamicContainer.setStyleSheetPseudoState$`
- `$API.ScriptDynamicContainer.setLocalLookAndFeel$`

---

## setStyleSheetProperty

**Signature:** `undefined setStyleSheetProperty(String variableId, NotUndefined value, String type)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setStyleSheetProperty("bg", Colours.red, "color");`

**Description:**
Sets a CSS variable on this component that can be queried from a stylesheet. The `type` parameter determines how the value is converted to a CSS-compatible string.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| variableId | String | no | The CSS variable name to set | -- |
| value | NotUndefined | no | The value to assign | Type must match the conversion |
| type | String | no | The unit/type conversion to apply | See value descriptions below |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| "path" | Converts a Path object to a base64-encoded string |
| "color" | Converts an integer colour to a CSS "#AARRGGBB" string |
| "%" | Converts a number to a percentage string (0.5 becomes "50%") |
| "px" | Converts a number to a pixel value string |
| "em" | Converts a number to an em value string |
| "vh" | Converts a number to a viewport-height string |
| "deg" | Converts a number to a degree string |
| "" | No conversion -- stores the value as-is |

**Cross References:**
- `$API.ScriptDynamicContainer.setStyleSheetClass$`
- `$API.ScriptDynamicContainer.setStyleSheetPseudoState$`

---

## setStyleSheetPseudoState

**Signature:** `undefined setStyleSheetPseudoState(String pseudoState)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setStyleSheetPseudoState(":hover");`

**Description:**
Sets one or more CSS pseudo-state selectors on this component. Multiple states can be combined (e.g. ":hover:active"). Pass an empty string "" to clear all pseudo-states. Automatically calls `sendRepaintMessage()` after setting the state.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| pseudoState | String | no | One or more CSS pseudo-state selectors | Can combine multiple; "" to clear |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| ":first-child" | First child pseudo-class |
| ":last-child" | Last child pseudo-class |
| ":root" | Root element pseudo-class |
| ":hover" | Mouse hover state |
| ":active" | Active/pressed state |
| ":focus" | Keyboard focus state |
| ":disabled" | Disabled state |
| ":hidden" | Hidden state |
| ":checked" | Checked/toggled state |

**Cross References:**
- `$API.ScriptDynamicContainer.setStyleSheetClass$`
- `$API.ScriptDynamicContainer.setStyleSheetProperty$`

---

## setTooltip

**Disabled:** property-deactivated
**Disabled Reason:** The `tooltip` property is deactivated on ScriptDynamicContainer. The container is a layout component with no tooltip support.

---

## setValue

**Signature:** `undefined setValue(NotUndefined newValue)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setValue(0.5);`

**Description:**
Sets the container's own ScriptComponent value. Thread-safe -- can be called from any thread. This sets the container's own value, not dyncomp child values. Use ContainerChild's `setValue()` method for individual child component values.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newValue | NotUndefined | no | The new value to set | Must not be a String |

**Pitfalls:**
- Do NOT pass a String value. Reports a script error.
- If called during `onInit`, the value will NOT be restored after recompilation.

**Cross References:**
- `$API.ScriptDynamicContainer.getValue$`
- `$API.ScriptDynamicContainer.setValueWithUndo$`
- `$API.ScriptDynamicContainer.setValueCallback$`

---

## setValueCallback

**Signature:** `undefined setValueCallback(Function valueFunction)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Creates a WeakCallbackHolder and sets up a ValueTree listener on the data model's Values tree.
**Minimal Example:** `{obj}.setValueCallback(onContainerValue);`

**Description:**
Registers a callback that fires whenever any dyncomp child component's value changes. The callback receives the component ID and the new value as arguments. Requires `setData()` to have been called first -- silently does nothing if the data model has not been created yet. The callback is registered with synchronous mode and high priority, so it fires immediately when a value changes on any thread.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| valueFunction | Function | no | A function with 2 parameters: componentId (String) and value (var) | Must be a JavaScript function |

**Callback Signature:** valueFunction(componentId: String, newValue: var)

**Pitfalls:**
- [BUG] Silently does nothing if called before `setData()`. The callback requires the data model's Values tree, which does not exist until `setData()` creates it.

**Cross References:**
- `$API.ScriptDynamicContainer.setData$`
- `$API.ScriptDynamicContainer.setControlCallback$`

**Example:**
```javascript:dyncomp-value-callback
// Title: Register a value callback for dynamic container children
const var dc = Content.addDynamicContainer("Container1", 0, 0);
dc.setPosition(0, 0, 300, 100);

const var vol = dc.setData({
    "id": "Vol",
    "type": "Slider",
    "x": 10, "y": 10, "width": 128, "height": 48
});

var lastId = "";
var lastValue = -1;

inline function onContainerValue(id, value)
{
    lastId = id;
    lastValue = value;
};

dc.setValueCallback(onContainerValue);

// --- test-only ---
vol.setValue(0.75);
// --- end test-only ---
```
```json:testMetadata:dyncomp-value-callback
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "lastId", "value": "Vol"},
    {"type": "REPL", "expression": "lastValue", "value": 0.75}
  ]
}
```

---

## setValueNormalized

**Disabled:** redundant
**Disabled Reason:** Base implementation simply calls `setValue(normalizedValue)` with no normalization mapping. Only meaningful on ScriptSlider, which maps the 0..1 range to the slider's configured min/max.

---

## setValueWithUndo

**Signature:** `undefined setValueWithUndo(NotUndefined newValue)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setValueWithUndo(0.5);`

**Description:**
Sets the container's own value through the undo manager, creating an `UndoableControlEvent`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newValue | NotUndefined | no | The new value to set with undo support | Must not be a String |

**Pitfalls:**
- Do NOT call this from `onControl` callbacks. It is intended for user-initiated value changes that should be undoable.

**Cross References:**
- `$API.ScriptDynamicContainer.setValue$`

---

## setZLevel

**Signature:** `undefined setZLevel(String zLevel)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setZLevel("AlwaysOnTop");`

**Description:**
Sets the depth level for this component among its siblings. Reports a script error if the value is not one of the four valid strings (case-sensitive).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| zLevel | String | no | The depth level for this component | Must be one of the four valid values (case-sensitive) |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| "Back" | Renders behind all sibling components |
| "Default" | Normal rendering order |
| "Front" | Renders in front of normal siblings |
| "AlwaysOnTop" | Always renders on top of all siblings |

---

## showControl

**Signature:** `undefined showControl(Integer shouldBeVisible)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.showControl(1);`

**Description:**
Sets the `visible` property with change message notification.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shouldBeVisible | Integer | no | Whether the component should be visible | 1 = show, 0 = hide |

**Cross References:**
- `$API.ScriptDynamicContainer.fadeComponent$`

---

## updateValueFromProcessorConnection

**Disabled:** property-deactivated
**Disabled Reason:** The `processorId` and `parameterId` properties are deactivated on ScriptDynamicContainer. No processor connection can be established on the container itself.
