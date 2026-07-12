## addToMacroControl

**Property Links:**
- Equivalent: none
- Related: set("macroControl", macroIndex), get("macroControl")

**Disabled:** property-deactivated
**Disabled Reason:** The macroControl property is deactivated for ScriptedViewport. The component does not support macro controller assignments.

## changed

**Signature:** `undefined changed()`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.changed();`

**Description:**
Triggers the control callback (either the custom one set via `setControlCallback` or the default `onControl` callback). Also notifies any registered value listeners.

**Pitfalls:**
- Cannot be called during `onInit` -- if called during `onInit`, it logs a console message and returns without executing.
- If `deferControlCallback` is set, the callback is deferred to the message thread.
- If the callback function throws an error, further script execution after the `changed()` call is aborted.

**Property Links:**
- Equivalent: none
- Related: deferControlCallback

**Interaction Notes:**
- If `deferControlCallback` is enabled, callback execution is deferred to the message thread.

**Cross References:**
- `$API.ScriptedViewport.setControlCallback$`
- `$API.ScriptedViewport.getValue$`

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

**Property Links:**
- Equivalent: none
- Related: set("visible", shouldBeVisible), get("visible")

**Cross References:**
- `$API.ScriptedViewport.showControl$`

## get

**Signature:** `var get(String propertyName)`
**Return Type:** `var`
**Call Scope:** warning
**Call Scope Note:** String involvement, atomic ref-count operations
**Minimal Example:** `var v = {obj}.get("text");`

**Description:**
Returns the current value of the named property. If the property is set on the component's value tree, returns that value; otherwise returns the default. Reports a script error if the property does not exist. ScriptedViewport adds these properties beyond the base set: `scrollBarThickness`, `autoHide`, `useList`, `viewPositionX`, `viewPositionY`, `items`, `fontName`, `fontSize`, `fontStyle`, `alignment`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| propertyName | String | no | The name of a component property to retrieve | Must be a valid property ID for this component type |

**Property Links:**
- Equivalent: canonical property getter API (`get("<propertyId>")`)
- Related: ScriptComponent.set

**Cross References:**
- `$API.ScriptedViewport.set$`
- `$API.ScriptedViewport.getAllProperties$`

## getAllProperties

**Signature:** `Array getAllProperties()`
**Return Type:** `Array`
**Call Scope:** unsafe
**Minimal Example:** `var props = {obj}.getAllProperties();`

**Description:**
Returns an array of strings containing all active (non-deactivated) property IDs for this component. For ScriptedViewport, the deactivated properties `macroControl`, `min`, and `max` are excluded from the result.

**Cross References:**
- `$API.ScriptedViewport.get$`
- `$API.ScriptedViewport.set$`

## getChildComponents

**Signature:** `Array getChildComponents()`
**Return Type:** `Array`
**Call Scope:** unsafe
**Minimal Example:** `var children = {obj}.getChildComponents();`

**Description:**
Returns an array of ScriptComponent references for all child components (components whose `parentComponent` is set to this component). Does not include the component itself in the result.

## getGlobalPositionX

**Signature:** `Integer getGlobalPositionX()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var x = {obj}.getGlobalPositionX();`

**Description:**
Returns the absolute x-position relative to the interface root, computed by recursively adding parent component x-offsets.

**Property Links:**
- Equivalent: none
- Related: get("x"), get("parentComponent")

**Cross References:**
- `$API.ScriptedViewport.getGlobalPositionY$`

## getGlobalPositionY

**Signature:** `Integer getGlobalPositionY()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var y = {obj}.getGlobalPositionY();`

**Description:**
Returns the absolute y-position relative to the interface root, computed by recursively adding parent component y-offsets.

**Property Links:**
- Equivalent: none
- Related: get("y"), get("parentComponent")

**Cross References:**
- `$API.ScriptedViewport.getGlobalPositionX$`

## getHeight

**Signature:** `Integer getHeight()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var h = {obj}.getHeight();`

**Description:**
Returns the `height` property as an integer.

**Property Links:**
- Equivalent: get("height")
- Related: set("height", value), setPosition(...)
## getId

**Signature:** `String getId()`
**Return Type:** `String`
**Call Scope:** warning
**Call Scope Note:** String involvement, atomic ref-count operations
**Minimal Example:** `var id = {obj}.getId();`

**Description:**
Returns the component's ID as a string (the variable name used when creating the component, e.g. `"Viewport1"`).

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

**Property Links:**
- Equivalent: none
- Related: get("width"), get("height")
## getOriginalRowIndex

**Signature:** `Integer getOriginalRowIndex(Integer rowIndex)`
**Return Type:** `Integer`
**Call Scope:** warning
**Call Scope Note:** O(n) indexOf over row data; consider caching for large tables
**Minimal Example:** `var orig = {obj}.getOriginalRowIndex(0);`

**Description:**
Returns the index of the given row in the original (unsorted) data array that was passed to `setTableRowData()`. This is useful when the table is sorted -- display row indices no longer match the original data order, and this method maps from the current display index back to the original index. Uses `SimpleReadWriteLock` for thread-safe access.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| rowIndex | Integer | no | The row index in the current (possibly sorted) display order | Must be within 0..numRows-1 |

**Pitfalls:**
- Requires `setTableMode()` to have been called first. Reports a script error if no table model exists.
- Uses object identity (`indexOf`) to find the original row. If two row objects are structurally identical, the first match is returned.

**Cross References:**
- `$API.ScriptedViewport.setTableRowData$`
- `$API.ScriptedViewport.setTableSortFunction$`

## getValue

**Signature:** `var getValue()`
**Return Type:** `var`
**Call Scope:** safe
**Minimal Example:** `var v = {obj}.getValue();`

**Description:**
Returns the current value of the component. The meaning depends on the active mode: in List mode, returns the selected row index (integer). In Table mode with `MultiColumnMode`, returns a `[column, row]` array after `setValue([c, r])` has been called. In Viewport mode, returns whatever was last set via `setValue()`. Uses a `SimpleReadWriteLock` for thread-safe read access.

**Pitfalls:**
- The stored value must not be a String. If it is, an assertion fires in debug builds.

**Cross References:**
- `$API.ScriptedViewport.setValue$`
- `$API.ScriptedViewport.getValueNormalized$`

## getValueNormalized

**Disabled:** redundant
**Disabled Reason:** Base implementation returns getValue() directly. Only meaningful on ScriptSlider which maps the actual value back to the 0..1 range.

## getWidth

**Signature:** `Integer getWidth()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var w = {obj}.getWidth();`

**Description:**
Returns the `width` property as an integer.

**Property Links:**
- Equivalent: get("width")
- Related: set("width", value), setPosition(...)
## grabFocus

**Signature:** `undefined grabFocus()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.grabFocus();`

**Description:**
Notifies z-level listeners that the component wants to grab keyboard focus. Only notifies the first listener (exclusive operation).

**Cross References:**
- `$API.ScriptedViewport.loseFocus$`

## loseFocus

**Signature:** `undefined loseFocus()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.loseFocus();`

**Description:**
Notifies all z-level listeners that the component wants to lose keyboard focus. Triggers the `wantsToLoseFocus()` callback on all registered `ZLevelListener` instances.

**Cross References:**
- `$API.ScriptedViewport.grabFocus$`

## sendRepaintMessage

**Signature:** `undefined sendRepaintMessage()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.sendRepaintMessage();`

**Description:**
Sends an asynchronous repaint message via `repaintBroadcaster`. This is useful when you've changed visual properties programmatically and need to force a UI redraw.

## set

**Signature:** `undefined set(String propertyName, NotUndefined value)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.set("text", "Hello");`

**Description:**
Sets a component property to the given value. Reports a script error if the property does not exist. During `onInit`, changes are applied without UI notification; outside `onInit`, sends change notifications to update the UI. ScriptedViewport adds these properties beyond the base set: `scrollBarThickness`, `autoHide`, `useList`, `viewPositionX`, `viewPositionY`, `items`, `fontName`, `fontSize`, `fontStyle`, `alignment`. Setting `viewPositionX` or `viewPositionY` broadcasts the new scroll position to the underlying viewport.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| propertyName | String | no | The property identifier to set | Must be a valid property ID for this component type |
| value | NotUndefined | no | The new value for the property | Type must match the property's expected type |

**Property Links:**
- Equivalent: canonical property setter API (`set("<propertyId>", value)`)
- Related: ScriptComponent.get

**Cross References:**
- `$API.ScriptedViewport.get$`
- `$API.ScriptedViewport.getAllProperties$`

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
| listOfKeys | NotUndefined | no | Key descriptions to consume -- a string, object, or array | See value descriptions and callback properties below |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| "all" | Catch all key presses exclusively (prevents parent from receiving them) |
| "all_nonexclusive" | Catch all key presses non-exclusively (parent still receives them) |

**Callback Properties:**

JSON object format for individual key descriptions:

| Property | Type | Description |
|----------|------|-------------|
| keyCode | int | The JUCE key code (required, must be non-zero) |
| shift | bool | Whether Shift modifier is required |
| cmd | bool | Whether Cmd/Ctrl modifier is required (also accepts "ctrl") |
| alt | bool | Whether Alt modifier is required |
| character | String | Optional character for the key press |

**Pitfalls:**
- Must be called BEFORE `setKeyPressCallback`. Reports a script error if an invalid key description is provided.

**Cross References:**
- `$API.ScriptedViewport.setKeyPressCallback$`

**Example:**
```javascript:basic
const var Viewport1 = Content.addViewport("Viewport1", 0, 0);

var keyLog = [];

// Consume all key presses exclusively
Viewport1.setConsumedKeyPresses("all");

Viewport1.setKeyPressCallback(function(event)
{
    if (!event.isFocusChange)
        keyLog.push(event.description);
});

// --- test-only ---
Console.testCallback(Viewport1, "setKeyPressCallback", {
    "isFocusChange": false,
    "character": "A",
    "specialKey": false,
    "keyCode": 65,
    "description": "A"
});
// --- end test-only ---
```
```json:testMetadata:basic
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "keyLog.length", "value": 1},
    {"type": "REPL", "expression": "keyLog[0]", "value": "A"}
  ]
}
```

## setControlCallback

**Signature:** `undefined setControlCallback(Function controlFunction)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setControlCallback(onMyControl);`

**Description:**
Assigns a custom inline function as the control callback, replacing the default `onControl` handler for this component. Pass `false` to revert to the default `onControl` callback. In List mode, the callback value is the selected row index (integer). In Table mode with MultiColumnMode, the callback value is a [column, row] array.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| controlFunction | Function | no | An inline function with 2 parameters (component, value), or false to clear | Must be an inline function with exactly 2 parameters |

**Callback Signature:** controlFunction(component: ScriptComponent, value: var)

**Pitfalls:**
- The function MUST be declared with `inline function`. Regular function references are rejected with a script error.
- Must have exactly 2 parameters. Reports a script error if the parameter count is wrong.
- Passing `false` clears the custom callback, reverting to the default `onControl` callback.

**Property Links:**
- Equivalent: none
- Related: processorId, parameterId

**Interaction Notes:**
- If `processorId` and `parameterId` are configured for processor forwarding, this custom callback path is bypassed.

**Cross References:**
- `$API.ScriptedViewport.changed$`

**Example:**
```javascript:basic
const var Viewport1 = Content.addViewport("Viewport1", 0, 0);
Viewport1.set("useList", true);
Viewport1.set("items", "Item A\nItem B\nItem C");
Viewport1.set("saveInPreset", false);

reg controlLog = [];

inline function onViewportChanged(component, value)
{
    controlLog.push(value);
};

Viewport1.setControlCallback(onViewportChanged);
```
```json:testMetadata:basic
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "Viewport1.setValue(1) || Viewport1.changed()", "value": false},
    {"type": "REPL", "expression": "controlLog[0]", "value": 1}
  ]
}
```

## setEventTypesForValueCallback

**Signature:** `undefined setEventTypesForValueCallback(Array eventTypeList)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setEventTypesForValueCallback(["Selection", "DoubleClick"]);`

**Description:**
Specifies which table event types trigger the parent component's `setValue()` callback (the value callback that enables undo support and value propagation). By default, `SingleClick`, `DoubleClick`, `ReturnKey`, and `SpaceKey` trigger the value callback. Only a subset of event types are legal for this purpose. Requires table mode to be active.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| eventTypeList | Array | no | Array of event type name strings | Must be an array of valid, non-illegal event type strings |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| "Selection" | Row selection changed |
| "SingleClick" | A cell received a single click |
| "DoubleClick" | A cell received a double click |
| "ReturnKey" | Return/Enter key was pressed on a selected row |

**Pitfalls:**
- Requires `setTableMode()` to have been called first. Reports a script error otherwise.
- Only "Selection", "SingleClick", "DoubleClick", and "ReturnKey" are legal. Passing "SliderCallback", "ButtonCallback", "SetValue", "Undo", or "DeleteRow" reports a script error.
- The eventTypeList must be an array. Passing a non-array value reports a script error.

**Property Links:**
- Equivalent: none
- Related: table/list configuration payload

**Cross References:**
- `$API.ScriptedViewport.setTableMode$`
- `$API.ScriptedViewport.setTableCallback$`
- `$API.ScriptedViewport.setValue$`

## setKeyPressCallback

**Signature:** `undefined setKeyPressCallback(Function keyboardFunction)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setKeyPressCallback(onKeyPress);`

**Description:**
Registers a callback that fires when a consumed key is pressed while this component has focus. MUST call `setConsumedKeyPresses()` BEFORE calling this method. Reports a script error if `setConsumedKeyPresses` has not been called yet.

The callback receives an event object with two possible shapes depending on whether it is a key press or a focus change event.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| keyboardFunction | Function | no | An inline function with 1 parameter (event object) | Must be an inline function |

**Callback Signature:** keyboardFunction(event: Object)

**Callback Properties:**

Key press event object:

| Property | Type | Description |
|----------|------|-------------|
| isFocusChange | bool | Always false for key events |
| character | String | The printable character, or "" for non-printable keys |
| specialKey | bool | true if not a printable character |
| isWhitespace | bool | true if the character is whitespace |
| isLetter | bool | true if the character is a letter |
| isDigit | bool | true if the character is a digit |
| keyCode | int | The JUCE key code |
| description | String | Human-readable description of the key press |
| shift | bool | true if Shift is held |
| cmd | bool | true if Cmd/Ctrl is held |
| alt | bool | true if Alt is held |

Focus change event object:

| Property | Type | Description |
|----------|------|-------------|
| isFocusChange | bool | Always true for focus events |
| hasFocus | bool | true if the component gained focus, false if lost |

**Pitfalls:**
- MUST call `setConsumedKeyPresses()` BEFORE calling this method. Reports a script error if `setConsumedKeyPresses` has not been called yet.

**Cross References:**
- `$API.ScriptedViewport.setConsumedKeyPresses$`

**Example:**
```javascript:basic
const var Viewport1 = Content.addViewport("Viewport1", 0, 0);
Viewport1.setConsumedKeyPresses("all");

reg keyLog = [];

inline function onKeyPress(event)
{
    if (!event.isFocusChange)
        keyLog.push(event.description);
};

Viewport1.setKeyPressCallback(onKeyPress);

// --- test-only ---
Console.testCallback(Viewport1, "setKeyPressCallback", {
    "isFocusChange": false,
    "character": "A",
    "specialKey": false,
    "isWhitespace": false,
    "isLetter": true,
    "isDigit": false,
    "keyCode": 65,
    "description": "A",
    "shift": false,
    "cmd": false,
    "alt": false
});
// --- end test-only ---
```
```json:testMetadata:basic
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "keyLog.length", "value": 1},
    {"type": "REPL", "expression": "keyLog[0]", "value": "A"}
  ]
}
```

## setLocalLookAndFeel

**Signature:** `undefined setLocalLookAndFeel(ScriptObject lafObject)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setLocalLookAndFeel(laf);`

**Description:**
Attaches a scripted look and feel object to this component and all its children. Pass `false` to clear it. For ScriptedViewport in table mode, the relevant LAF functions are `drawTableRowBackground`, `drawTableCell`, `drawTableHeaderBackground`, and `drawTableHeaderColumn`. The `drawScrollbar` function applies to the scrollbar in all modes.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| lafObject | ScriptObject | no | A ScriptedLookAndFeel object, or false to clear | Must be a ScriptedLookAndFeel instance |

**Pitfalls:**
- Propagates to ALL child components automatically.
- If the LAF uses CSS (has a stylesheet), automatically calls `setStyleSheetClass({})` to initialize the class selector.

**Cross References:**
- `$API.ScriptedViewport.setStyleSheetClass$`
- `$API.ScriptedViewport.setStyleSheetProperty$`
- `$API.ScriptedViewport.setStyleSheetPseudoState$`

**Example:**
```javascript:basic
const var Viewport1 = Content.addViewport("Viewport1", 0, 0);
const var laf = Content.createLocalLookAndFeel();

laf.registerFunction("drawTableRowBackground", function(g, obj)
{
    g.fillAll(obj.selected ? obj.itemColour : obj.bgColour);
});

laf.registerFunction("drawTableCell", function(g, obj)
{
    g.setColour(obj.textColour);
    g.setFont("Arial", 14);
    g.drawAlignedText(obj.text, obj.area, "left");
});

Viewport1.setLocalLookAndFeel(laf);
```
```json:testMetadata:basic
{
  "testable": false
}
```

## setPosition

**Signature:** `undefined setPosition(Integer x, Integer y, Integer w, Integer h)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setPosition(10, 10, 200, 50);`

**Description:**
Sets the component's position and size in one call. Directly sets the `x`, `y`, `width`, `height` properties on the property tree.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| x | Integer | no | X position in pixels, relative to parent | 0-900 |
| y | Integer | no | Y position in pixels, relative to parent | 0-MAX_SCRIPT_HEIGHT |
| w | Integer | no | Width in pixels | 0-900 |
| h | Integer | no | Height in pixels | 0-MAX_SCRIPT_HEIGHT |

**Property Links:**
- Equivalent: none
- Related: set("x", x), set("y", y), set("width", w), set("height", h)
## setPropertiesFromJSON

**Disabled:** no-op
**Disabled Reason:** This method is declared in the ScriptComponent header but is NOT registered as a direct API method on component instances. It is exposed on the Content object as Content.setPropertiesFromJSON(componentName, jsonData) instead.

## setStyleSheetClass

**Signature:** `undefined setStyleSheetClass(String classIds)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setStyleSheetClass(".active");`

**Description:**
Sets the CSS class selectors for this component. The component's own type class (derived from the `type` property, lowercased, prefixed with `.`) is automatically prepended. Creates the `ComponentStyleSheetProperties` value tree if it does not yet exist.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| classIds | String | no | Space-separated CSS class selectors to apply | e.g. ".myClass .highlighted" |

**Cross References:**
- `$API.ScriptedViewport.setStyleSheetProperty$`
- `$API.ScriptedViewport.setStyleSheetPseudoState$`
- `$API.ScriptedViewport.setLocalLookAndFeel$`

## setStyleSheetProperty

**Signature:** `undefined setStyleSheetProperty(String variableId, NotUndefined value, String type)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setStyleSheetProperty("bg", Colours.red, "color");`

**Description:**
Sets a CSS variable on this component that can be queried from a stylesheet. The `type` parameter determines how the value is converted to a CSS-compatible string. Creates the `ComponentStyleSheetProperties` value tree if it does not yet exist.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| variableId | String | no | The CSS variable name to set | -- |
| value | NotUndefined | no | The value to assign | Type must match the conversion specified by the type parameter |
| type | String | no | The unit/type conversion to apply before storing | See value descriptions below |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| "path" | Converts a Path object to a base64-encoded string |
| "color" | Converts an integer colour to a CSS "#AARRGGBB" string |
| "%" | Converts a number to a percentage string (0.5 becomes "50%") |
| "px" | Converts a number to a pixel value string (10 becomes "10px") |
| "em" | Converts a number to an em value string |
| "vh" | Converts a number to a viewport-height string |
| "deg" | Converts a number to a degree string |
| "" | No conversion -- stores the value as-is |

**Cross References:**
- `$API.ScriptedViewport.setStyleSheetClass$`
- `$API.ScriptedViewport.setStyleSheetPseudoState$`
- `$API.ScriptedViewport.setLocalLookAndFeel$`

## setStyleSheetPseudoState

**Signature:** `undefined setStyleSheetPseudoState(String pseudoState)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setStyleSheetPseudoState(":hover");`

**Description:**
Sets one or more CSS pseudo-state selectors on this component. Multiple states can be combined in one string (e.g. `":hover:active"`, `":checked:focus"`). Pass an empty string `""` to clear all pseudo-states. Automatically calls `sendRepaintMessage()` after setting the state.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| pseudoState | String | no | One or more CSS pseudo-state selectors to apply | Can combine multiple states; pass "" to clear |

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
- `$API.ScriptedViewport.setStyleSheetClass$`
- `$API.ScriptedViewport.setStyleSheetProperty$`
- `$API.ScriptedViewport.setLocalLookAndFeel$`

## setTableCallback

**Signature:** `undefined setTableCallback(Function callbackFunction)`
**Return Type:** `undefined`
**Call Scope:** init
**Minimal Example:** `{obj}.setTableCallback(onTableEvent);`

**Description:**
Registers a callback function that is notified for all user interactions with the table. The callback receives a single argument -- a JSON object with `Type`, `rowIndex`, `columnID`, and `value` properties. Must be called in `onInit` after `setTableMode()`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| callbackFunction | Function | no | An inline function with 1 parameter (event object) | Must be a JavaScript function |

**Callback Signature:** callbackFunction(event: Object)

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| Type | String | The event type string: "Click", "DoubleClick", "Selection", "ReturnKey", "SpaceKey", "DeleteRow", "Slider", "Button", "ComboBox", "SetValue" |
| rowIndex | int | The row index in the current display order, or -1 for background click |
| columnID | String | The ID property from the column metadata for the clicked column |
| value | var | Depends on event type: full row data object for Click/DoubleClick/Selection/ReturnKey/DeleteRow, slider value for Slider, toggle state for Button, combobox selection for ComboBox |

**Pitfalls:**
- Must be called in `onInit`. Reports a script error if called at runtime.
- Requires `setTableMode()` to have been called first. Reports a script error if no table model exists.
- In MultiColumnMode, Selection and SingleClick events are deferred to an async update to coalesce rapid notifications.

**Property Links:**
- Equivalent: none
- Related: table/list configuration payload

**Cross References:**
- `$API.ScriptedViewport.setTableMode$`
- `$API.ScriptedViewport.setTableColumns$`
- `$API.ScriptedViewport.setTableRowData$`
- `$API.ScriptedViewport.setEventTypesForValueCallback$`

**DiagramRef:** table-setup

**Example:**
```javascript:basic
const var Viewport1 = Content.addViewport("Viewport1", 0, 0);

Viewport1.setTableMode({ "RowHeight": 24 });
Viewport1.setTableColumns([
    { "ID": "Name", "Width": 150 },
    { "ID": "Active", "Type": "Button", "Width": 80, "Toggle": true }
]);

inline function onTableEvent(event)
{
    if (event.Type == "Click")
        Console.print("Clicked row " + event.rowIndex + ", column: " + event.columnID);

    if (event.Type == "Button")
        Console.print("Button toggled: " + event.value);
};

Viewport1.setTableCallback(onTableEvent);
```
```json:testMetadata:basic
{
  "testable": false
}
```

## setTableColumns

**Signature:** `undefined setTableColumns(Array columnMetadata)`
**Return Type:** `undefined`
**Call Scope:** init
**Minimal Example:** `{obj}.setTableColumns(myColumns);`

**Description:**
Defines the columns of the table. Must be called in `onInit` after `setTableMode()`. Each element in the array is a JSON object describing one column. The `ID` property is required and is used to look up cell values from row data objects.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| columnMetadata | Array | no | Array of column definition objects | Must be called in onInit, after setTableMode() |

**Callback Properties:**

Column definition object properties:

| Property | Type | Description |
|----------|------|-------------|
| ID | String | Column identifier, used to look up values in row data objects (required) |
| Label | String | Display name in the header (defaults to ID if omitted) |
| Type | String | Cell type: "Text" (default), "Button", "Slider", "ComboBox", "Hidden" |
| Width | int | Column width in pixels |
| MinWidth | int | Minimum column width (default 1) |
| MaxWidth | int | Maximum column width (-1 for unlimited) |
| Visible | bool | Whether column is visible (default true) |
| PeriodicRepaint | bool | If true, column repaints on a timer (default false) |
| Focus | bool | Whether column can receive keyboard focus for arrow navigation (default true) |
| Text | String | Button label text (default "Button") or ComboBox placeholder (default "No selection") |
| Toggle | bool | Button: false = momentary, true = toggle (default false) |
| ValueMode | String | ComboBox value mode: "ID" (default), "Index", or "Text" |
| items | Array | ComboBox dropdown items (can also be set per-row in row data) |
| MinValue | double | Slider range minimum (default 0.0). Property name depends on SliderRangeIdSet. |
| MaxValue | double | Slider range maximum (default 1.0). Property name depends on SliderRangeIdSet. |
| StepSize | double | Slider step size, 0 = continuous (default 0.0). Property name depends on SliderRangeIdSet. |
| SkewFactor | double | Slider range skew, 1.0 = linear (default 1.0). Property name depends on SliderRangeIdSet. |
| suffix | String | Slider suffix text (default "") |
| defaultValue | double | Slider double-click return value |
| showTextBox | bool | Slider: enables shift-click text input (default true) |
| style | String | Slider style: "Knob" (default), "Horizontal", "Vertical" |

**Pitfalls:**
- Must be called in `onInit`. Reports a script error if called at runtime.
- Requires `setTableMode()` to have been called first. Reports a script error if no table model exists.
- The column `ID` values must match the property keys in the row data objects passed to `setTableRowData()`.

**Property Links:**
- Equivalent: none
- Related: table/list configuration payload

**Cross References:**
- `$API.ScriptedViewport.setTableMode$`
- `$API.ScriptedViewport.setTableRowData$`
- `$API.ScriptedViewport.setTableCallback$`

**DiagramRef:** table-setup

**Example:**
```javascript:basic
Viewport1.setTableColumns([
    { "ID": "Name", "Label": "Name", "Width": 200 },
    { "ID": "Volume", "Type": "Slider", "Width": 120, "style": "Horizontal" },
    { "ID": "Mute", "Type": "Button", "Width": 60, "Toggle": true, "Text": "M" },
    { "ID": "Output", "Type": "ComboBox", "Width": 100, "ValueMode": "ID" }
]);
```
```json:testMetadata:basic
{
  "testable": false
}
```

## setTableMode

**Signature:** `undefined setTableMode(JSON tableMetadata)`
**Return Type:** `undefined`
**Call Scope:** init
**Minimal Example:** `{obj}.setTableMode({});`

**Description:**
Activates table mode for this viewport by creating the internal `ScriptTableListModel`. Must be called in `onInit` before `setTableColumns()` and `setTableCallback()`. The metadata object configures table-wide behavior including row height, header height, sorting, and selection mode.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| tableMetadata | JSON | no | Table configuration object | Must be called in onInit |

**Callback Properties:**

Table metadata properties:

| Property | Type | Description |
|----------|------|-------------|
| RowHeight | int | Height of each row in pixels (default 20) |
| HeaderHeight | int | Height of the table header in pixels (default 24) |
| Sortable | bool | Enables column header click-to-sort (default false) |
| MultiColumnMode | bool | Enables [column, row] value tracking and undo support (default false) |
| MultiSelection | bool | Enables multiple row selection (default false) |
| ScrollOnDrag | bool | Enables scroll-on-drag for touch devices (default false) |
| ProcessSpaceKey | bool | Enables SpaceKey event handling (default false) |
| CallbackOnSliderDrag | bool | If true, slider cells fire callbacks during drag; if false, only on release (default true) |
| SliderRangeIdSet | String | Range ID set for slider cells: "scriptnode" (default), "ScriptComponent", "MidiAutomation", "MidiAutomationFull" |

**Pitfalls:**
- Must be called in `onInit`. Reports a script error if called at runtime.
- Once called, the viewport is permanently in table mode for this compilation. Cannot switch back to viewport or list mode without recompiling.

**Property Links:**
- Equivalent: none
- Related: table/list configuration payload

**Cross References:**
- `$API.ScriptedViewport.setTableColumns$`
- `$API.ScriptedViewport.setTableRowData$`
- `$API.ScriptedViewport.setTableCallback$`
- `$API.ScriptedViewport.setTableSortFunction$`
- `$API.ScriptedViewport.setEventTypesForValueCallback$`

**DiagramRef:** table-setup

**Example:**
```javascript:basic
const var Viewport1 = Content.addViewport("Viewport1", 0, 0);
Viewport1.set("width", 400);
Viewport1.set("height", 300);

Viewport1.setTableMode({
    "RowHeight": 28,
    "HeaderHeight": 30,
    "Sortable": true,
    "MultiColumnMode": true,
    "MultiSelection": false
});
```
```json:testMetadata:basic
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "Viewport1.getWidth()", "value": 400},
    {"type": "REPL", "expression": "Viewport1.getHeight()", "value": 300}
  ]
}
```

## setTableRowData

**Signature:** `undefined setTableRowData(Array tableData)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setTableRowData(myData);`

**Description:**
Updates the row data for the table. Each element in the array must be a JSON object whose property keys match the column `ID` values defined in `setTableColumns()`. The data is cloned internally -- the original array is preserved for `getOriginalRowIndex()` lookups. If sorting is active, the new data is automatically re-sorted.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| tableData | Array | no | Array of row data objects with properties matching column IDs | Must be called after setTableMode() |

**Pitfalls:**
- Requires `setTableMode()` to have been called first. Reports a script error if no table model exists.
- The data is cloned on assignment. Modifications to the original array after calling this method are not reflected in the table.
- Can be called at any time (not restricted to onInit), allowing dynamic row updates at runtime.

**Property Links:**
- Equivalent: none
- Related: table/list configuration payload

**Cross References:**
- `$API.ScriptedViewport.setTableMode$`
- `$API.ScriptedViewport.setTableColumns$`
- `$API.ScriptedViewport.getOriginalRowIndex$`

**DiagramRef:** table-setup

**Example:**
```javascript:basic
// Update table data at runtime
var data = [];
for (i = 0; i < 10; i++)
{
    data.push({
        "Name": "Item " + (i + 1),
        "Value": Math.random(),
        "Active": true
    });
}

Viewport1.setTableRowData(data);
```
```json:testMetadata:basic
{
  "testable": false
}
```

## setTableSortFunction

**Signature:** `undefined setTableSortFunction(Function sortFunction)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setTableSortFunction(mySort);`

**Description:**
Sets a custom comparator function used when the user clicks a column header to sort the table (requires `Sortable: true` in table metadata). The function receives two cell values from the sort column and must return an integer: negative if the first value should come before the second, positive if after, and 0 if equal. The sort function is called synchronously. Passing a non-function reverts to the default alphabetic/numeric sorter.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| sortFunction | Function | no | An inline function with 2 parameters (value1, value2) returning an integer | Must return negative, 0, or positive integer |

**Callback Signature:** sortFunction(a: var, b: var)

**Pitfalls:**
- Requires `setTableMode()` to have been called first. Reports a script error if no table model exists.
- The sort function is called synchronously via `callSync` -- it must not perform long-running operations.
- The function receives individual cell values from the sort column, not full row objects.

**Property Links:**
- Equivalent: none
- Related: table/list configuration payload

**Cross References:**
- `$API.ScriptedViewport.setTableMode$`
- `$API.ScriptedViewport.getOriginalRowIndex$`

**Example:**
```javascript:basic
// Case-insensitive string sort
inline function caseInsensitiveSort(a, b)
{
    local aLower = a.toLowerCase();
    local bLower = b.toLowerCase();

    if (aLower < bLower) return -1;
    if (aLower > bLower) return 1;
    return 0;
};

Viewport1.setTableSortFunction(caseInsensitiveSort);
```
```json:testMetadata:basic
{
  "testable": false
}
```

## setTooltip

**Signature:** `undefined setTooltip(String tooltip)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setTooltip("Hover text");`

**Description:**
Sets the tooltip text to display on mouse hover.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| tooltip | String | no | The tooltip text to display on mouse hover | -- |

**Property Links:**
- Equivalent: set("tooltip", tooltip)
- Related: get("tooltip")
## setValueNormalized

**Disabled:** redundant
**Disabled Reason:** Base implementation just calls setValue(). Only meaningful on ScriptSlider which maps 0..1 to the configured range.

## setValueWithUndo

**Signature:** `undefined setValueWithUndo(NotUndefined newValue)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setValueWithUndo(0.5);`

**Description:**
Sets the value through the undo manager, creating an `UndoableControlEvent`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newValue | NotUndefined | no | The new value to set with undo support | Must not be a String |

**Pitfalls:**
- Do NOT call this from `onControl` callbacks. It is intended for user-initiated value changes that should be undoable.

**Property Links:**
- Equivalent: none
- Related: useUndoManager

**Interaction Notes:**
- Undo integration depends on `useUndoManager`; if disabled, undo history integration is not active.

**Cross References:**
- `$API.ScriptedViewport.setValue$`

## setZLevel

**Signature:** `undefined setZLevel(String zLevel)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setZLevel("AlwaysOnTop");`

**Description:**
Sets the depth level for this component among its siblings. Reports a script error if the value is not one of the four valid strings (case-sensitive). Notifies all z-level listeners when the level changes.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| zLevel | String | no | The depth level for this component among its siblings | Must be one of the four valid values (case-sensitive) |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| "Back" | Renders behind all sibling components |
| "Default" | Normal rendering order |
| "Front" | Renders in front of normal siblings |
| "AlwaysOnTop" | Always renders on top of all siblings |

## setValue

**Signature:** `undefined setValue(NotUndefined newValue)`
**Return Type:** `undefined`
**Call Scope:** warning
**Call Scope Note:** Safe in list/viewport mode; allocates UndoableTableSelection in table MultiColumnMode
**Minimal Example:** `{obj}.setValue(0);`

**Description:**
Sets the component's value. In table mode with `MultiColumnMode` enabled, passing a 2-element array `[column, row]` triggers the table's selection callback via an `UndoableTableSelection` action (respects the `useUndoManager` property). In list mode, pass an integer row index to select a row. Always calls the base `ScriptComponent.setValue()` at the end, which handles thread-safe storage, UI update, and linked component propagation.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newValue | NotUndefined | no | The new value: integer row index (list mode), [column, row] array (table MultiColumnMode), or arbitrary value (viewport mode) | Must not be a String |

**Pitfalls:**
- Do NOT pass a String value. Reports a script error.
- If called during `onInit`, the value will NOT be restored after recompilation.
- In table MultiColumnMode, the [column, row] array triggers a SetValue callback on the table model, which may be skipped if the same cell is already selected.

**Property Links:**
- Equivalent: none
- Related: linkedTo

**Interaction Notes:**
- Value propagation can forward to linked components through the `linkedTo` routing setup.

**Cross References:**
- `$API.ScriptedViewport.getValue$`
- `$API.ScriptedViewport.setValueWithUndo$`
- `$API.ScriptedViewport.setTableCallback$`

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

**Property Links:**
- Equivalent: none
- Related: set("visible", shouldBeVisible), get("visible")

**Cross References:**
- `$API.ScriptedViewport.fadeComponent$`

## updateValueFromProcessorConnection

**Signature:** `undefined updateValueFromProcessorConnection()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.updateValueFromProcessorConnection();`

**Description:**
Reads the current attribute value from the connected processor (set via the `processorId` and `parameterId` properties) and calls `setValue()` with that value. Does nothing if no processor connection is established.

**Property Links:**
- Equivalent: none
- Related: get("processorId"), get("parameterId"), setValue(...)

**Cross References:**
- `$API.ScriptedViewport.setValue$`
