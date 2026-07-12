## addToMacroControl

**Signature:** `undefined addToMacroControl(Integer macroIndex)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.addToMacroControl(0);`

**Description:**
Assigns this slider pack to a macro controller slot.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| macroIndex | Integer | no | Macro controller index | 0-7 |

---

## fadeComponent

**Signature:** `undefined fadeComponent(Integer shouldBeVisible, Integer milliseconds)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.fadeComponent(1, 250);`

**Description:**
Animates visibility changes using the global UI animator.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shouldBeVisible | Integer | no | Target visibility flag | 1 = show, 0 = hide |
| milliseconds | Integer | no | Fade duration in milliseconds | > 0 |

---

## get

**Signature:** `var get(String propertyName)`
**Return Type:** `NotUndefined`
**Call Scope:** safe
**Minimal Example:** `var v = {obj}.get("sliderAmount");`

**Description:**
Returns the current value of a property on this slider pack component.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| propertyName | String | no | Property identifier to read | Must be a valid ScriptSliderPack property |

**Cross References:**
- `$API.ScriptSliderPack.set$`
- `$API.ScriptSliderPack.getAllProperties$`

---

## getAllProperties

**Signature:** `Array getAllProperties()`
**Return Type:** `Array`
**Call Scope:** unsafe
**Minimal Example:** `var props = {obj}.getAllProperties();`

**Description:**
Returns all active property IDs for this component, including ScriptSliderPack-specific properties.

**Cross References:**
- `$API.ScriptSliderPack.get$`
- `$API.ScriptSliderPack.set$`

---

## getChildComponents

**Signature:** `Array getChildComponents()`
**Return Type:** `Array`
**Call Scope:** unsafe
**Minimal Example:** `var children = {obj}.getChildComponents();`

**Description:**
Returns child script components whose `parentComponent` points to this slider pack.

---

## getDataAsBuffer

**Signature:** `Buffer getDataAsBuffer()`
**Return Type:** `Buffer`
**Call Scope:** safe
**Minimal Example:** `var b = {obj}.getDataAsBuffer();`

**Description:**
Returns a direct buffer reference to the slider pack data.

**Pitfalls:**
- Editing the returned buffer directly bypasses `setAllValues()` / `setSliderAtIndex()` callback flow.

**Cross References:**
- `$API.ScriptSliderPack.setAllValues$`
- `$API.ScriptSliderPack.setSliderAtIndex$`

---

## getGlobalPositionX

**Signature:** `Integer getGlobalPositionX()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var x = {obj}.getGlobalPositionX();`

**Description:**
Returns the absolute x position relative to the interface root.

---

## getGlobalPositionY

**Signature:** `Integer getGlobalPositionY()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var y = {obj}.getGlobalPositionY();`

**Description:**
Returns the absolute y position relative to the interface root.

---

## getHeight

**Signature:** `Integer getHeight()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var h = {obj}.getHeight();`

**Description:**
Returns the current component height in pixels.

---

## getId

**Signature:** `String getId()`
**Return Type:** `String`
**Call Scope:** safe
**Minimal Example:** `var id = {obj}.getId();`

**Description:**
Returns the component ID used when the slider pack was created.

---

## getLocalBounds

**Signature:** `Array getLocalBounds(Double reduceAmount)`
**Return Type:** `Array`
**Call Scope:** safe
**Minimal Example:** `var bounds = {obj}.getLocalBounds(0.0);`

**Description:**
Returns local bounds reduced by the given inset amount.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| reduceAmount | Double | no | Inset amount in pixels for all sides | >= 0.0 |

**Cross References:**
- `$API.ScriptSliderPack.setPosition$`
- `$API.ScriptSliderPack.getWidth$`
- `$API.ScriptSliderPack.getHeight$`

---

## getNumSliders

**Signature:** `Integer getNumSliders()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var n = {obj}.getNumSliders();`

**Description:**
Returns the number of sliders in the currently bound slider-pack data source.

**Pitfalls:**
- Returns `0` when no valid slider-pack data source is resolved.

**Cross References:**
- `$API.ScriptSliderPack.set$`
- `$API.ScriptSliderPack.setWidthArray$`
- `$API.ScriptSliderPack.referToData$`

---

## getSliderValueAt

**Signature:** `Double getSliderValueAt(Integer index)`
**Return Type:** `Double`
**Call Scope:** unsafe
**Minimal Example:** `var v = {obj}.getSliderValueAt(0);`

**Description:**
Returns a slider value at the given index and updates the displayed index highlight.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| index | Integer | no | Slider index to read | 0 to `getNumSliders() - 1` |

**Pitfalls:**
- Out-of-range indices return the slider-pack default value instead of throwing an error.

**Cross References:**
- `$API.ScriptSliderPack.setSliderAtIndex$`
- `$API.ScriptSliderPack.setAllValues$`

---

## getValueNormalized

**Disabled:** redundant
**Disabled Reason:** Inherited base implementation just returns `getValue()` and is not meaningful for multi-value slider-pack data.

---

## getWidth

**Signature:** `Integer getWidth()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var w = {obj}.getWidth();`

**Description:**
Returns the current component width in pixels.

---

## grabFocus

**Signature:** `undefined grabFocus()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.grabFocus();`

**Description:**
Requests keyboard focus for this component via z-level listeners.

**Cross References:**
- `$API.ScriptSliderPack.loseFocus$`
- `$API.ScriptSliderPack.setKeyPressCallback$`

---

## loseFocus

**Signature:** `undefined loseFocus()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.loseFocus();`

**Description:**
Requests focus release for this component via z-level listeners.

**Cross References:**
- `$API.ScriptSliderPack.grabFocus$`
- `$API.ScriptSliderPack.setKeyPressCallback$`

---

## referToData

**Signature:** `undefined referToData(Object sliderPackData)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.referToData(spData);`

**Description:**
Rebinds the component to another slider-pack data source, another compatible complex-data component, or `-1` to restore its internal data object.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| sliderPackData | Object | no | Data source reference or reset token | `ScriptSliderPackData`, compatible complex-data component, or `-1` |

**Pitfalls:**
- [BUG] Unsupported argument types are silently ignored, leaving the previous binding active.

**Cross References:**
- `$API.ScriptSliderPack.registerAtParent$`
- `$API.ScriptSliderPack.getNumSliders$`

---

## registerAtParent

**Signature:** `Object registerAtParent(Integer index)`
**Return Type:** `Object`
**Call Scope:** unsafe
**Minimal Example:** `var dataRef = {obj}.registerAtParent(0);`

**Description:**
Registers this slider-pack data object in the parent processor's dynamic external-data pool and returns a `ScriptSliderPackData` handle when registration succeeds.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| index | Integer | no | External data slot index | >= 0 |

**Pitfalls:**
- [BUG] Returns `undefined` without an error when the parent processor does not support dynamic external data.

**Cross References:**
- `$API.ScriptSliderPack.referToData$`
- `$API.ScriptSliderPack.set$`

---

## sendRepaintMessage

**Signature:** `undefined sendRepaintMessage()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.sendRepaintMessage();`

**Description:**
Sends an asynchronous repaint request for this component.

---

## set

**Signature:** `undefined set(String propertyName, NotUndefined value)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.set("sliderAmount", 32);`

**Description:**
Sets a ScriptSliderPack property value.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| propertyName | String | no | Property identifier to write | Must be a valid ScriptSliderPack property |
| value | NotUndefined | no | New property value | Must match property type |

**Cross References:**
- `$API.ScriptSliderPack.get$`
- `$API.ScriptSliderPack.getAllProperties$`

---

## setAllValueChangeCausesCallback

**Signature:** `undefined setAllValueChangeCausesCallback(Integer shouldBeEnabled)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setAllValueChangeCausesCallback(0);`

**Description:**
Enables or disables control-callback triggering for non-undo bulk/single write helpers.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shouldBeEnabled | Integer | no | Toggle for callback behavior | 1 = enabled, 0 = disabled |

**Cross References:**
- `$API.ScriptSliderPack.setAllValues$`
- `$API.ScriptSliderPack.setSliderAtIndex$`
- `$API.ScriptSliderPack.setAllValuesWithUndo$`

---

## setAllValues

**Signature:** `undefined setAllValues(NotUndefined value)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setAllValues([0.0, 0.5, 1.0]);`

**Description:**
Writes slider values in bulk. Accepts a Number (fill all), Array, or Buffer (copy matching indices).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| value | AudioData | no | Scalar fill value or per-slider values | Buffer / Array values beyond slider count are ignored |

**Pitfalls:**
- If the Array/Buffer is shorter than `getNumSliders()`, only the available indices are updated and remaining sliders keep their previous values.

**Cross References:**
- `$API.ScriptSliderPack.setSliderAtIndex$`
- `$API.ScriptSliderPack.setAllValuesWithUndo$`
- `$API.ScriptSliderPack.setAllValueChangeCausesCallback$`

**Example:**
```javascript:bulk-fill-slider-pack
// Title: Fill every slider with one scalar value
const var spData = Engine.createAndRegisterSliderPackData(0);
spData.setNumSliders(4);
spData.setAllValues(0.0);

const var sp = Content.addSliderPack("SP1", 0, 0);
sp.set("sliderAmount", 4);
sp.referToData(spData);
sp.setAllValues(0.75);
```
```json:testMetadata:bulk-fill-slider-pack
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "sp.getNumSliders()", "value": 4},
    {"type": "REPL", "expression": "sp.getSliderValueAt(0)", "value": 0.75},
    {"type": "REPL", "expression": "sp.getSliderValueAt(3)", "value": 0.75}
  ]
}
```

---

## setAllValuesWithUndo

**Signature:** `undefined setAllValuesWithUndo(NotUndefined value)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setAllValuesWithUndo([0.2, 0.4, 0.8]);`

**Description:**
Bulk value write with undo integration.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| value | AudioData | no | Scalar fill value or per-slider values | Buffer / Array values are copied into an undo action |

**Pitfalls:**
- [BUG] This method always triggers callback/content-change notification, even when `setAllValueChangeCausesCallback(false)` was set.

**Cross References:**
- `$API.ScriptSliderPack.setAllValues$`
- `$API.ScriptSliderPack.setAllValueChangeCausesCallback$`

---

## setConsumedKeyPresses

**Signature:** `undefined setConsumedKeyPresses(NotUndefined listOfKeys)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setConsumedKeyPresses("all");`

**Description:**
Defines which key presses this component consumes before key-press callbacks are enabled.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| listOfKeys | NotUndefined | no | Key selection as String, Object, or Array | Must be valid JUCE key descriptions |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| "all" | Consume all keys exclusively |
| "all_nonexclusive" | Consume all keys but allow parent propagation |

**Cross References:**
- `$API.ScriptSliderPack.setKeyPressCallback$`

---

## setControlCallback

**Signature:** `undefined setControlCallback(Function controlFunction)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setControlCallback(onSliderPackControl);`

**Description:**
Assigns a custom inline control callback for this slider pack.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| controlFunction | Function | yes | Inline callback to replace default `onControl` handling | Must be inline and accept exactly 2 args |

**Callback Signature:** controlFunction(component: ScriptComponent, value: var)

**Pitfalls:**
- Callback must be declared with `inline function` and exactly two parameters.

**Cross References:**
- `$API.ScriptSliderPack.setAllValueChangeCausesCallback$`
- `$API.ScriptSliderPack.setAllValues$`
- `$API.ScriptSliderPack.setSliderAtIndex$`

**Example:**
```javascript:slider-pack-control-callback
// Title: Custom control callback for slider-pack changes
const var sp = Content.addSliderPack("SP2", 0, 0);
reg lastIndex = -1;

inline function onSliderPackControl(component, value)
{
    lastIndex = value;
}

sp.setControlCallback(onSliderPackControl);
```
```json:testMetadata:slider-pack-control-callback
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "sp.setSliderAtIndex(1, 0.5) || true", "value": true},
    {"type": "REPL", "expression": "lastIndex", "value": 1},
    {"type": "REPL", "expression": "sp.getSliderValueAt(1)", "value": 0.5}
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
Registers a key-press callback for this component. Requires `setConsumedKeyPresses()` first.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| keyboardFunction | Function | yes | Inline callback invoked for consumed key/focus events | Must be inline and consume one event object |

**Callback Signature:** keyboardFunction(event: Object)

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| isFocusChange | bool | `true` for focus events, `false` for key events |
| character | String | Printable character, or empty string for non-printable keys |
| specialKey | bool | `true` for non-printable keys |
| keyCode | int | JUCE key code |
| description | String | Human-readable key description |
| shift | bool | Shift modifier state |
| cmd | bool | Cmd/Ctrl modifier state |
| alt | bool | Alt modifier state |
| hasFocus | bool | Present on focus events, indicates focus gained/lost |

**Pitfalls:**
- Must call `setConsumedKeyPresses()` before this method.

**Cross References:**
- `$API.ScriptSliderPack.setConsumedKeyPresses$`
- `$API.ScriptSliderPack.grabFocus$`
- `$API.ScriptSliderPack.loseFocus$`

**Example:**
```javascript:slider-pack-keypress-callback
// Title: Handle consumed key presses on a slider-pack
const var sp = Content.addSliderPack("SPKey", 0, 0);
sp.setConsumedKeyPresses("all");
reg lastKeyCode = -1;

inline function onKeyPress(event)
{
    if (!event.isFocusChange)
        lastKeyCode = event.keyCode;
}

sp.setKeyPressCallback(onKeyPress);

// --- test-only ---
Console.testCallback(sp, "setKeyPressCallback", {
    "isFocusChange": false,
    "character": "A",
    "specialKey": false,
    "keyCode": 65,
    "description": "A",
    "shift": false,
    "cmd": false,
    "alt": false
});
// --- end test-only ---
```
```json:testMetadata:slider-pack-keypress-callback
{
  "testable": true,
  "verifyScript": {"type": "REPL", "expression": "lastKeyCode", "value": 65}
}
```

---

## setLocalLookAndFeel

**Signature:** `undefined setLocalLookAndFeel(ScriptObject lafObject)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setLocalLookAndFeel(laf);`

**Description:**
Attaches a local scripted look-and-feel object to this slider pack.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| lafObject | ScriptObject | no | Scripted look-and-feel object, or `false` to clear | Must be a `ScriptedLookAndFeel` object |

**Pitfalls:**
- Setting a local LAF also propagates to child components.

---

## setPosition

**Signature:** `undefined setPosition(Integer x, Integer y, Integer w, Integer h)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setPosition(10, 10, 220, 90);`

**Description:**
Sets component position and size in one call.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| x | Integer | no | Left position in pixels | 0-900 |
| y | Integer | no | Top position in pixels | 0-MAX_SCRIPT_HEIGHT |
| w | Integer | no | Width in pixels | 0-900 |
| h | Integer | no | Height in pixels | 0-MAX_SCRIPT_HEIGHT |

**Cross References:**
- `$API.ScriptSliderPack.getLocalBounds$`
- `$API.ScriptSliderPack.getWidth$`
- `$API.ScriptSliderPack.getHeight$`

---

## setPropertiesFromJSON

**Disabled:** no-op
**Disabled Reason:** Declared on `ScriptComponent` but not registered as a callable method on component instances. Use `Content.setPropertiesFromJSON(componentId, jsonData)` instead.

---

## setSliderAtIndex

**Signature:** `undefined setSliderAtIndex(Integer index, Double value)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setSliderAtIndex(2, 0.75);`

**Description:**
Sets a single slider value in the currently bound slider-pack data source.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| index | Integer | no | Slider index to update | 0 to `getNumSliders() - 1` |
| value | Double | no | New slider value | Typically within current min/max range |

**Cross References:**
- `$API.ScriptSliderPack.getSliderValueAt$`
- `$API.ScriptSliderPack.setAllValues$`
- `$API.ScriptSliderPack.setAllValueChangeCausesCallback$`

---

## setStyleSheetClass

**Signature:** `undefined setStyleSheetClass(String classIds)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setStyleSheetClass(".active .large");`

**Description:**
Sets additional CSS class selectors for this component.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| classIds | String | no | Space-separated CSS classes | Component type class is prepended automatically |

**Cross References:**
- `$API.ScriptSliderPack.setStyleSheetProperty$`
- `$API.ScriptSliderPack.setStyleSheetPseudoState$`

---

## setStyleSheetProperty

**Signature:** `undefined setStyleSheetProperty(String variableId, NotUndefined value, String type)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setStyleSheetProperty("accent", Colours.red, "color");`

**Description:**
Sets a component stylesheet variable, with optional CSS conversion.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| variableId | String | no | CSS variable identifier | Non-empty string |
| value | NotUndefined | no | Value to store | Must match selected conversion |
| type | String | no | Conversion type | See value descriptions |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| "path" | Convert Path object to base64 string |
| "color" | Convert colour to `#AARRGGBB` |
| "%" | Convert number to percentage string |
| "px" | Convert number to pixel unit |
| "em" | Convert number to em unit |
| "vh" | Convert number to viewport-height unit |
| "deg" | Convert number to degree unit |
| "" | Store value without conversion |

**Cross References:**
- `$API.ScriptSliderPack.setStyleSheetClass$`
- `$API.ScriptSliderPack.setStyleSheetPseudoState$`

---

## setStyleSheetPseudoState

**Signature:** `undefined setStyleSheetPseudoState(String pseudoState)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setStyleSheetPseudoState(":hover:active");`

**Description:**
Sets CSS pseudo-state flags for this component and triggers repaint.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| pseudoState | String | no | Pseudo-state selector string | Multiple states can be concatenated |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| ":first-child" | First-child pseudo-state |
| ":last-child" | Last-child pseudo-state |
| ":root" | Root pseudo-state |
| ":hover" | Hover pseudo-state |
| ":active" | Pressed/active pseudo-state |
| ":focus" | Focus pseudo-state |
| ":disabled" | Disabled pseudo-state |
| ":hidden" | Hidden pseudo-state |
| ":checked" | Checked pseudo-state |

**Cross References:**
- `$API.ScriptSliderPack.setStyleSheetClass$`
- `$API.ScriptSliderPack.setStyleSheetProperty$`

---

## setTooltip

**Signature:** `undefined setTooltip(String tooltip)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setTooltip("Drag to edit steps");`

**Description:**
Sets tooltip text shown when hovering over the component.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| tooltip | String | no | Tooltip text | -- |

---

## setUsePreallocatedLength

**Signature:** `undefined setUsePreallocatedLength(Integer numMaxSliders)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setUsePreallocatedLength(128);`

**Description:**
Enables fixed-capacity preallocation for slider-pack storage to reduce reallocations during resizing.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| numMaxSliders | Integer | no | Maximum preallocated slider count (0 disables preallocation) | >= 0 |

**Pitfalls:**
- When preallocation is active, effective slider count cannot exceed `numMaxSliders`.

**Cross References:**
- `$API.ScriptSliderPack.set$`
- `$API.ScriptSliderPack.getNumSliders$`

---

## setValueNormalized

**Disabled:** redundant
**Disabled Reason:** Inherited base implementation just forwards to `setValue(normalizedValue)` and does not provide normalized mapping semantics for slider-pack arrays.

---

## setValueWithUndo

**Disabled:** redundant
**Disabled Reason:** `ScriptSliderPack` uses dedicated bulk undo handling via `setAllValuesWithUndo`; base scalar undo semantics are not a good fit for slider-pack array data.

---

## setWidthArray

**Signature:** `undefined setWidthArray(Array normalizedWidths)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setWidthArray([0.0, 0.25, 0.6, 1.0]);`

**Description:**
Sets non-uniform slider widths using normalized cumulative breakpoints.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| normalizedWidths | Array | no | Cumulative width breakpoints | Array length should be `getNumSliders() + 1` |

**Pitfalls:**
- Length mismatch logs an error but still stores the array; rendering falls back to equal-width layout when the size does not match `numSliders + 1`.

**Cross References:**
- `$API.ScriptSliderPack.set$`
- `$API.ScriptSliderPack.getNumSliders$`

**Example:**
```javascript:slider-pack-width-map
// Title: Use a custom width map for uneven slider spacing
const var sp = Content.addSliderPack("SPWidth", 0, 0);
sp.set("sliderAmount", 3);
sp.setWidthArray([0.0, 0.2, 0.7, 1.0]);
```
```json:testMetadata:slider-pack-width-map
{
  "testable": false,
  "skipReason": "Layout geometry is visual and not directly verifiable via REPL without wrapper internals"
}
```

---

## setZLevel

**Signature:** `undefined setZLevel(String zLevel)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setZLevel("Front");`

**Description:**
Changes this component's z-order layer among siblings.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| zLevel | String | no | Z-level selector | Must be one of `Back`, `Default`, `Front`, `AlwaysOnTop` |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| "Back" | Render behind sibling components |
| "Default" | Use normal layer |
| "Front" | Render in front of default siblings |
| "AlwaysOnTop" | Render above all siblings |

---

## showControl

**Signature:** `undefined showControl(Integer shouldBeVisible)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.showControl(1);`

**Description:**
Shows or hides the component by updating the `visible` property.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shouldBeVisible | Integer | no | Visibility flag | 1 = show, 0 = hide |

**Cross References:**
- `$API.ScriptSliderPack.fadeComponent$`

---

## updateValueFromProcessorConnection

**Disabled:** no-op
**Disabled Reason:** `ScriptSliderPack` does not use the scalar `processorId`/`parameterId` value-link path, so this inherited refresh helper is not meaningful here.
