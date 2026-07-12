# ScriptButton -- Method Documentation

---

## addToMacroControl

**Signature:** `undefined addToMacroControl(Integer macroIndex)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.addToMacroControl(0);`

**Description:**
Assigns this button to a macro controller slot. Sets the internal `connectedMacroIndex`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| macroIndex | Integer | no | The macro controller index | 0-7 |

**Property Links:**
- Equivalent: none
- Related: set("macroControl", macroIndex), get("macroControl")

---
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
- `$API.ScriptButton.setControlCallback$`
- `$API.ScriptButton.getValue$`
- `$API.ScriptButton.setValue$`

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

**Property Links:**
- Equivalent: none
- Related: set("visible", shouldBeVisible), get("visible")

**Cross References:**
- `$API.ScriptButton.showControl$`

---

## get

**Signature:** `var get(String propertyName)`
**Return Type:** `var`
**Call Scope:** safe
**Minimal Example:** `var v = {obj}.get("radioGroup");`

**Description:**
Returns the current value of the named property. If the property is set on the component's value tree, returns that value; otherwise returns the default. Reports a script error if the property does not exist.

Base properties available on all components: `text`, `visible`, `enabled`, `locked`, `x`, `y`, `width`, `height`, `min`, `max`, `defaultValue`, `tooltip`, `bgColour`, `itemColour`, `itemColour2`, `textColour`, `macroControl`, `saveInPreset`, `isPluginParameter`, `pluginParameterName`, `pluginParameterGroup`, `deferControlCallback`, `isMetaParameter`, `linkedTo`, `automationId`, `useUndoManager`, `parentComponent`, `processorId`, `parameterId`. ScriptButton adds: `filmstripImage`, `numStrips`, `isVertical`, `scaleFactor`, `radioGroup`, `isMomentary`, `enableMidiLearn`, `setValueOnClick`, `mouseCursor`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| propertyName | String | no | The name of a component property to retrieve | Must be a valid property ID for this component type |

**Property Links:**
- Equivalent: canonical property getter API (`get("<propertyId>")`)
- Related: ScriptComponent.set

**Cross References:**
- `$API.ScriptButton.set$`
- `$API.ScriptButton.getAllProperties$`
- `$API.ScriptButton.setPropertiesFromJSON$`

---

## getAllProperties

**Signature:** `Array getAllProperties()`
**Return Type:** `Array`
**Call Scope:** unsafe
**Minimal Example:** `var props = {obj}.getAllProperties();`

**Description:**
Returns an array of strings containing all active (non-deactivated) property IDs for this component. Includes both base ScriptComponent properties and ScriptButton-specific properties. Note that `min` and `max` are deactivated on ScriptButton and will not appear in the result.

**Cross References:**
- `$API.ScriptButton.get$`
- `$API.ScriptButton.set$`
- `$API.ScriptButton.setPropertiesFromJSON$`

---

## getChildComponents

**Signature:** `Array getChildComponents()`
**Return Type:** `Array`
**Call Scope:** unsafe
**Minimal Example:** `var children = {obj}.getChildComponents();`

**Description:**
Returns an array of ScriptComponent references for all child components (components whose `parentComponent` is set to this component). Does not include the component itself in the result.

---

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
- `$API.ScriptButton.getGlobalPositionY$`

---

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
- `$API.ScriptButton.getGlobalPositionX$`

---

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

**Cross References:**
- `$API.ScriptButton.getWidth$`
- `$API.ScriptButton.getLocalBounds$`
- `$API.ScriptButton.setPosition$`

---

## getId

**Signature:** `String getId()`
**Return Type:** `String`
**Call Scope:** safe
**Minimal Example:** `var id = {obj}.getId();`

**Description:**
Returns the component's ID as a string (the variable name used when creating the component, e.g. `"MyButton"`).

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

**Property Links:**
- Equivalent: none
- Related: get("width"), get("height")

**Cross References:**
- `$API.ScriptButton.getWidth$`
- `$API.ScriptButton.getHeight$`
- `$API.ScriptButton.setPosition$`

---

## getValue

**Signature:** `var getValue()`
**Return Type:** `var`
**Call Scope:** safe
**Minimal Example:** `var v = {obj}.getValue();`

**Description:**
Returns the current value of the button. For ScriptButton, this is 0 (off) or 1 (on). Uses a `SimpleReadWriteLock` for thread-safe read access.

**Pitfalls:**
- The stored value must not be a String. If it is, an assertion fires in debug builds.

**Cross References:**
- `$API.ScriptButton.setValue$`
- `$API.ScriptButton.getValueNormalized$`

---

## getValueNormalized

**Signature:** `Double getValueNormalized()`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var v = {obj}.getValueNormalized();`

**Description:**
Returns the normalized value (0.0 to 1.0). For ScriptButton, this is equivalent to `getValue()` since the button's range is already 0..1.

**Cross References:**
- `$API.ScriptButton.setValueNormalized$`
- `$API.ScriptButton.getValue$`

---

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

**Cross References:**
- `$API.ScriptButton.getHeight$`
- `$API.ScriptButton.getLocalBounds$`
- `$API.ScriptButton.setPosition$`

---

## grabFocus

**Signature:** `undefined grabFocus()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.grabFocus();`

**Description:**
Notifies z-level listeners that the component wants to grab keyboard focus. Only notifies the first listener (exclusive operation).

**Cross References:**
- `$API.ScriptButton.loseFocus$`

---

## loseFocus

**Signature:** `undefined loseFocus()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.loseFocus();`

**Description:**
Notifies all z-level listeners that the component wants to lose keyboard focus. Triggers the `wantsToLoseFocus()` callback on all registered `ZLevelListener` instances.

**Cross References:**
- `$API.ScriptButton.grabFocus$`

---

## sendRepaintMessage

**Signature:** `undefined sendRepaintMessage()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.sendRepaintMessage();`

**Description:**
Sends an asynchronous repaint message via `repaintBroadcaster`. This is useful when you've changed visual properties programmatically and need to force a UI redraw.

---

## set

**Signature:** `undefined set(String propertyName, NotUndefined value)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.set("isMomentary", 1);`

**Description:**
Sets a component property to the given value. Reports a script error if the property does not exist. During `onInit`, changes are applied without UI notification; outside `onInit`, sends change notifications to update the UI. Tracks which properties have been set by script for profiling purposes.

Base properties available on all components: `text`, `visible`, `enabled`, `locked`, `x`, `y`, `width`, `height`, `min`, `max`, `defaultValue`, `tooltip`, `bgColour`, `itemColour`, `itemColour2`, `textColour`, `macroControl`, `saveInPreset`, `isPluginParameter`, `pluginParameterName`, `pluginParameterGroup`, `deferControlCallback`, `isMetaParameter`, `linkedTo`, `automationId`, `useUndoManager`, `parentComponent`, `processorId`, `parameterId`. ScriptButton adds: `filmstripImage`, `numStrips`, `isVertical`, `scaleFactor`, `radioGroup`, `isMomentary`, `enableMidiLearn`, `setValueOnClick`, `mouseCursor`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| propertyName | String | no | The property identifier to set | Must be a valid property ID for this component type |
| value | NotUndefined | no | The new value for the property | Type must match the property's expected type |

**Property Links:**
- Equivalent: canonical property setter API (`set("<propertyId>", value)`)
- Related: ScriptComponent.get

**Cross References:**
- `$API.ScriptButton.get$`
- `$API.ScriptButton.getAllProperties$`
- `$API.ScriptButton.setPropertiesFromJSON$`

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
| listOfKeys | NotUndefined | no | Key descriptions to consume -- a string, object, or array | See value descriptions and callback properties below |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| "all" | Catch all key presses exclusively (prevents parent from receiving them) |
| "all_nonexclusive" | Catch all key presses non-exclusively (parent still receives them) |

When passing individual key descriptions (as a string, object, or array of either), each key description can be:
- A string using JUCE key description format (e.g. `"A"`, `"ctrl + S"`, `"F5"`, `"shift + tab"`). Uses JUCE's `KeyPress::createFromDescription()`.
- A JSON object with the following properties:

**Callback Properties:**

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
- `$API.ScriptButton.setKeyPressCallback$`

**Example:**
```javascript:consumed-key-formats
// Title: Three ways to specify consumed keys
const var btn = Content.addButton("KeyBtn", 0, 0);

// Consume all keys exclusively
btn.setConsumedKeyPresses("all");

// Override with specific keys by description string
btn.setConsumedKeyPresses(["ctrl + S", "F5", "escape"]);

// Override with specific keys by JSON object
btn.setConsumedKeyPresses([
    { "keyCode": 65, "ctrl": true },
    { "keyCode": 83, "ctrl": true }
]);
```
```json:testMetadata:consumed-key-formats
{
  "testable": false,
  "skipReason": "Key press consumption has no queryable state -- effect is only observable through keyboard interaction"
}
```

---

## setControlCallback

**Signature:** `undefined setControlCallback(Function controlFunction)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setControlCallback(onMyControl);`

**Description:**
Assigns a custom inline function as the control callback, replacing the default `onControl` handler for this component. Pass `undefined` to revert to the default `onControl` callback.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| controlFunction | Function | no | An inline function with 2 parameters (component, value), or undefined to clear | Must be an inline function with exactly 2 parameters |

**Callback Signature:** controlFunction(component: ScriptComponent, value: var)

**Pitfalls:**
- The function MUST be declared with `inline function`. Regular function references are rejected with a script error.
- Must have exactly 2 parameters. Reports a script error if the parameter count is wrong.
- Reports an error if the script processor has a DspNetwork that is forwarding controls to parameters.
- Passing `undefined` or empty `var()` clears the custom callback, reverting to the default `onControl` callback.

**Property Links:**
- Equivalent: none
- Related: processorId, parameterId

**Interaction Notes:**
- If `processorId` and `parameterId` are configured for processor forwarding, this custom callback path is bypassed.

**Cross References:**
- `$API.ScriptButton.changed$`

**Example:**
```javascript:control-callback-registration
// Title: Registering a custom control callback
const var btn = Content.addButton("CbBtn", 0, 0);
btn.set("saveInPreset", false);

var callbackLog = [];

inline function onButtonToggled(component, value)
{
    callbackLog.push(component.getId() + ":" + value);
};

btn.setControlCallback(onButtonToggled);

// --- test-only ---
btn.setValue(1);
btn.changed();
// --- end test-only ---
```
```json:testMetadata:control-callback-registration
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "btn.setValue(1) || btn.changed()", "value": false},
    {"type": "REPL", "expression": "callbackLog[0]", "value": "CbBtn:1.0"}
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
- `$API.ScriptButton.setConsumedKeyPresses$`

**Example:**
```javascript:key-press-callback
// Title: Handling key presses on a button
const var btn = Content.addButton("KeyPressBtn", 0, 0);
btn.setConsumedKeyPresses("all");

var lastKey = "";

inline function onKeyPress(event)
{
    if (!event.isFocusChange)
        lastKey = event.description;
};

btn.setKeyPressCallback(onKeyPress);

// --- test-only ---
Console.testCallback(btn, "setKeyPressCallback", {
    "isFocusChange": false,
    "character": "A",
    "specialKey": false,
    "keyCode": 65,
    "description": "A"
});
// --- end test-only ---
```
```json:testMetadata:key-press-callback
{
  "testable": true,
  "verifyScript": {"type": "REPL", "expression": "lastKey", "value": "A"}
}
```

---

## setLocalLookAndFeel

**Signature:** `undefined setLocalLookAndFeel(ScriptObject lafObject)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setLocalLookAndFeel(laf);`

**Description:**
Attaches a scripted look and feel object to this button and all its children. Pass `undefined` to clear it. The object must be a `ScriptedLookAndFeel` instance. If the object is not a valid LAF, the local look and feel is cleared. For ScriptButton, the relevant LAF function is `drawToggleButton`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| lafObject | ScriptObject | no | A ScriptedLookAndFeel object, or undefined to clear | Must be a ScriptedLookAndFeel instance |

**Pitfalls:**
- Propagates to ALL child components automatically -- iterates over all child ScriptComponent instances and sets their localLookAndFeel to the same object.
- If the LAF uses CSS (has a stylesheet), automatically calls `setStyleSheetClass({})` to initialize the class selector.
- When CSS mode is active, colour properties (bgColour, itemColour, itemColour2, textColour) are initialized in the property tree if not already present, and default-property-removal is disabled.
- A custom LAF via `setLocalLookAndFeel` takes priority over filmstrip rendering set via the `filmstripImage` property.

**Cross References:**
- `$API.ScriptButton.setStyleSheetClass$`
- `$API.ScriptButton.setStyleSheetProperty$`
- `$API.ScriptButton.setStyleSheetPseudoState$`

**Example:**
```javascript:laf-draw-toggle-button
const var btn = Content.addButton("ButtonName", 0, 0);
const var laf = Content.createLocalLookAndFeel();

laf.registerFunction("drawToggleButton", function(g, obj)
{
    g.fillAll(obj.value ? obj.itemColour1 : obj.bgColour);
    g.setColour(obj.textColour);
    g.drawAlignedText(obj.text, obj.area, "centred");
});

btn.setLocalLookAndFeel(laf);
```

```json:testMetadata:laf-draw-toggle-button
{
  "testable": false,
  "reason": "LAF rendering requires visual verification -- drawToggleButton callback executes during paint, not testable via REPL."
}
```

---

## setPosition

**Signature:** `undefined setPosition(Integer x, Integer y, Integer w, Integer h)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setPosition(10, 10, 128, 28);`

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

**Cross References:**
- `$API.ScriptButton.getWidth$`
- `$API.ScriptButton.getHeight$`
- `$API.ScriptButton.getLocalBounds$`

---

## setPropertiesFromJSON

**Signature:** `undefined setPropertiesFromJSON(JSON jsonData)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setPropertiesFromJSON({"text": "Enable", "visible": true});`

**Description:**
Sets multiple component properties at once from a JSON object. Each key in the object must be a valid property ID for this component type. This is a convenience method for batch property assignment.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| jsonData | JSON | no | A JSON object where keys are property names and values are property values | Keys must be valid property IDs |

**Cross References:**
- `$API.ScriptButton.set$`
- `$API.ScriptButton.get$`
- `$API.ScriptButton.getAllProperties$`

---

## setStyleSheetClass

**Signature:** `undefined setStyleSheetClass(String classIds)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setStyleSheetClass(".active");`

**Description:**
Sets the CSS class selectors for this component. The component's own type class (`.scriptbutton`) is automatically prepended. Creates the `ComponentStyleSheetProperties` value tree if it does not yet exist.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| classIds | String | no | Space-separated CSS class selectors to apply | e.g. ".myClass .highlighted" |

**Cross References:**
- `$API.ScriptButton.setStyleSheetProperty$`
- `$API.ScriptButton.setStyleSheetPseudoState$`
- `$API.ScriptButton.setLocalLookAndFeel$`

**Example:**
```javascript:stylesheet-class-assignment
const var btn = Content.addButton("ButtonName", 0, 0);

// Add custom classes (component type class is auto-prepended)
btn.setStyleSheetClass(".large .primary");
// Result: ".scriptbutton .large .primary"
```

```json:testMetadata:stylesheet-class-assignment
{
  "testable": false,
  "reason": "CSS class application requires stylesheet rendering context -- no observable state change accessible via REPL."
}
```

---

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
| variableId | String | no | The CSS variable name to set; becomes available as a CSS variable in stylesheets | -- |
| value | NotUndefined | no | The value to assign | Type must match the conversion specified by the type parameter |
| type | String | no | The unit/type conversion to apply before storing | See value descriptions below |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| "path" | Converts a Path object to a base64-encoded string (calls PathObject::toBase64()) |
| "color" | Converts an integer colour to a CSS "#AARRGGBB" string |
| "%" | Converts a number to a percentage string (0.5 becomes "50%") |
| "px" | Converts a number to a pixel value string (10 becomes "10px") |
| "em" | Converts a number to an em value string |
| "vh" | Converts a number to a viewport-height string |
| "deg" | Converts a number to a degree string |
| "" | No conversion -- stores the value as-is |

**Cross References:**
- `$API.ScriptButton.setStyleSheetClass$`
- `$API.ScriptButton.setStyleSheetPseudoState$`
- `$API.ScriptButton.setLocalLookAndFeel$`

**Example:**
```javascript:stylesheet-property-types
const var btn = Content.addButton("ButtonName", 0, 0);

// Set a colour variable
btn.setStyleSheetProperty("accent-color", 0xFFFF0000, "color");

// Set a size variable
btn.setStyleSheetProperty("border-size", 2, "px");
```

```json:testMetadata:stylesheet-property-types
{
  "testable": false,
  "reason": "CSS variable assignment requires stylesheet rendering context -- no observable state change accessible via REPL."
}
```

---

## setStyleSheetPseudoState

**Signature:** `undefined setStyleSheetPseudoState(String pseudoState)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setStyleSheetPseudoState(":checked");`

**Description:**
Sets one or more CSS pseudo-state selectors on this component. Multiple states can be combined in one string (e.g. `":hover:active"`, `":checked:focus"`). Pass an empty string `""` to clear all pseudo-states. Automatically calls `sendRepaintMessage()` after setting the state.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| pseudoState | String | no | One or more CSS pseudo-state selectors to apply | Can combine multiple states; pass "" to clear |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| ":first-child" | First child pseudo-class (bitmask 1) |
| ":last-child" | Last child pseudo-class (bitmask 2) |
| ":root" | Root element pseudo-class (bitmask 4) |
| ":hover" | Mouse hover state (bitmask 8) |
| ":active" | Active/pressed state (bitmask 16) |
| ":focus" | Keyboard focus state (bitmask 32) |
| ":disabled" | Disabled state (bitmask 64) |
| ":hidden" | Hidden state (bitmask 128) |
| ":checked" | Checked/toggled state (bitmask 256) |

**Cross References:**
- `$API.ScriptButton.setStyleSheetClass$`
- `$API.ScriptButton.setStyleSheetProperty$`
- `$API.ScriptButton.setLocalLookAndFeel$`

---

## setTooltip

**Signature:** `undefined setTooltip(String tooltip)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setTooltip("Toggle effect bypass");`

**Description:**
Sets the tooltip text to display on mouse hover.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| tooltip | String | no | The tooltip text to display on mouse hover | -- |

**Property Links:**
- Equivalent: set("tooltip", tooltip)
- Related: get("tooltip")

---
## setValue

**Signature:** `undefined setValue(NotUndefined newValue)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setValue(1);`

**Description:**
Sets the button's value. For ScriptButton, pass 1 for on or 0 for off. Thread-safe -- can be called from any thread; the UI update happens asynchronously. Propagates the value to all linked component targets. Triggers an async UI update via `triggerAsyncUpdate()`. Sends value listener messages.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newValue | NotUndefined | no | The new value to set | 0 (off) or 1 (on); must not be a String |

**Pitfalls:**
- Do NOT pass a String value. Reports a script error.
- If called during `onInit`, the value will NOT be restored after recompilation (`skipRestoring` is set to true).

**Property Links:**
- Equivalent: none
- Related: linkedTo

**Interaction Notes:**
- Value propagation can forward to linked components through the `linkedTo` routing setup.

**Cross References:**
- `$API.ScriptButton.getValue$`
- `$API.ScriptButton.changed$`
- `$API.ScriptButton.setValueNormalized$`
- `$API.ScriptButton.setValueWithUndo$`

---

## setValueNormalized

**Signature:** `undefined setValueNormalized(Double normalizedValue)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setValueNormalized(1.0);`

**Description:**
Sets the value using a normalized 0..1 range. For ScriptButton, this is equivalent to `setValue()` since the button's range is already 0..1. Pass 0.0 for off or 1.0 for on.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| normalizedValue | Double | no | A value in the range 0.0 to 1.0 | 0.0 (off) or 1.0 (on) |

**Cross References:**
- `$API.ScriptButton.getValueNormalized$`
- `$API.ScriptButton.setValue$`

---

## setValueWithUndo

**Signature:** `undefined setValueWithUndo(NotUndefined newValue)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setValueWithUndo(1);`

**Description:**
Sets the button's value through the undo manager, creating an `UndoableControlEvent`. For ScriptButton, pass 1 for on or 0 for off.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newValue | NotUndefined | no | The new value to set with undo support | 0 (off) or 1 (on); must not be a String |

**Pitfalls:**
- Do NOT call this from `onControl` callbacks. It is intended for user-initiated value changes that should be undoable.

**Property Links:**
- Equivalent: none
- Related: useUndoManager

**Interaction Notes:**
- Undo integration depends on `useUndoManager`; if disabled, undo history integration is not active.

**Cross References:**
- `$API.ScriptButton.setValue$`

---

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

**Property Links:**
- Equivalent: none
- Related: set("visible", shouldBeVisible), get("visible")

**Cross References:**
- `$API.ScriptButton.fadeComponent$`

---

## updateValueFromProcessorConnection

**Signature:** `undefined updateValueFromProcessorConnection()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.updateValueFromProcessorConnection();`

**Description:**
Reads the current attribute value from the connected processor (set via the `processorId` and `parameterId` properties) and calls `setValue()` with that value. Does nothing if no processor connection is established.

Special parameter index values for the `parameterId` property:
- `-2`: Reads modulation intensity from a Modulation processor
- `-3`: Reads bypass state (1.0 if bypassed, 0.0 if not)
- `-4`: Reads inverted bypass state (0.0 if bypassed, 1.0 if not)
- `>= 0`: Reads the attribute at the given parameter index

**Property Links:**
- Equivalent: none
- Related: get("processorId"), get("parameterId"), setValue(...)

**Cross References:**
- `$API.ScriptButton.setValue$`

---

## setPopupData

**Signature:** `undefined setPopupData(JSON jsonData, Array position)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setPopupData({"Type": "Keyboard"}, [0, 30, 400, 100]);`

**Description:**
Attaches a FloatingTile configuration to this button. When the button is clicked, it toggles a popup containing the configured FloatingTile at the specified position relative to the button. Clicking the button again dismisses the popup. The popup acts as a toggle -- click once to show, click again to dismiss. Buttons that are already inside a FloatingTilePopup will not show additional popups.

The `jsonData` parameter is a JSON object describing the FloatingTile content type and its configuration properties. This object is passed directly to the FloatingTile constructor. The `position` parameter specifies the popup's offset from the button and its dimensions as `[x, y, width, height]`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| jsonData | JSON | no | A FloatingTile configuration object with at least a "Type" property | Must be a valid FloatingTile content type configuration |
| position | Array | no | The popup position and size as [x, y, width, height] relative to the button | Must be a 4-element array of integers |

**Pitfalls:**
- The `position` parameter must be a 4-element array `[x, y, w, h]`. If the format is wrong, a script error is thrown: "position must be an array with this structure: [x, y, w, h]".
- The popup only appears on left-click. Buttons already inside a FloatingTilePopup will not create nested popups.

**Property Links:**
- Equivalent: none
- Related: popup configuration payload/state

**Cross References:**
- `$API.ScriptButton.setValue$`

**Example:**
```javascript:button-popup-keyboard
// Title: Attaching a Keyboard popup to a button
const var btn = Content.addButton("ShowKeyboard", 10, 10);
btn.set("text", "Keyboard");

btn.setPopupData({
    "Type": "Keyboard",
    "KeyWidth": 14,
    "LowKey": 36,
    "HiKey": 84
}, [0, 30, 500, 100]);
```
```json:testMetadata:button-popup-keyboard
{
  "testable": false,
  "skipReason": "Popup display requires mouse interaction to trigger and verify visually"
}
```

---
