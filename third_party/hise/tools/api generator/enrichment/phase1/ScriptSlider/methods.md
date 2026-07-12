# ScriptSlider -- Method Documentation

---

## addToMacroControl

**Signature:** `undefined addToMacroControl(Integer macroIndex)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.addToMacroControl(0);`

**Description:**
Assigns this slider to a macro controller slot. Sets the internal `connectedMacroIndex`.

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
Triggers the control callback (custom callback from `setControlCallback` or default `onControl`) and notifies registered value listeners.

**Pitfalls:**
- Cannot be called during `onInit` -- it logs a message and returns.
- If `deferControlCallback` is enabled, callback execution is deferred.
- If the callback throws, execution after `changed()` is aborted.

**Property Links:**
- Equivalent: none
- Related: deferControlCallback

**Interaction Notes:**
- If `deferControlCallback` is enabled, callback execution is deferred to the message thread.

**Cross References:**
- `$API.ScriptSlider.setControlCallback$`
- `$API.ScriptSlider.getValue$`

---

## connectToModulatedParameter

**Signature:** `undefined connectToModulatedParameter(String moduleId, NotUndefined parameterId)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.connectToModulatedParameter("SimpleGain1", "Gain");`

**Description:**
Connects the slider's modulation display query to a target processor parameter (or special modulation target names like `GainModulation` and `PitchModulation`). If possible, it also prepares matrix modulation popup data for the selected target.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| moduleId | String | no | Target processor ID | Must resolve to an existing processor |
| parameterId | NotUndefined | no | Parameter index (Integer) or parameter identifier (String) | Integer index or valid parameter name |

**Pitfalls:**
- If `moduleId` is not found, the method reports an error and clears the previously registered modulation display query function.

**Cross References:**
- `$API.ScriptSlider.updateValueFromProcessorConnection$`

---

## contains

**Signature:** `Integer contains(Number valueToCheck)`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var ok = {obj}.contains(0.5);`

**Description:**
Checks whether `valueToCheck` is inside the current range selection (`minimum..maximum`) for range sliders.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| valueToCheck | Number | no | Value to test against current range bounds | Numeric value |

**Cross References:**
- `$API.ScriptSlider.setStyle$`
- `$API.ScriptSlider.setMinValue$`
- `$API.ScriptSlider.setMaxValue$`
- `$API.ScriptSlider.getMinValue$`
- `$API.ScriptSlider.getMaxValue$`

---

## createModifiers

**Signature:** `ScriptObject createModifiers()`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Minimal Example:** `var mods = {obj}.createModifiers();`

**Description:**
Creates and returns a `Modifiers` script object containing constants used with `setModifiers(action, modifiers)`.

**Cross References:**
- `$API.ScriptSlider.setModifiers$`

---

## fadeComponent

**Signature:** `undefined fadeComponent(Integer shouldBeVisible, Integer milliseconds)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.fadeComponent(1, 250);`

**Description:**
Toggles visibility with a fade animation over the given duration. Only sends a fade message if visibility actually changes.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shouldBeVisible | Integer | no | Target visibility state | 1 = show, 0 = hide |
| milliseconds | Integer | no | Fade duration in milliseconds | > 0 |

**Property Links:**
- Equivalent: none
- Related: set("visible", shouldBeVisible), get("visible")

---
## get

**Signature:** `var get(String propertyName)`
**Return Type:** `var`
**Call Scope:** safe
**Minimal Example:** `var v = {obj}.get("mode");`

**Description:**
Returns the current value of the named property or its default value when unset. Reports an error if the property does not exist.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| propertyName | String | no | Property name to retrieve | Must be a valid ScriptSlider property |

**Property Links:**
- Equivalent: canonical property getter API (`get("<propertyId>")`)
- Related: ScriptComponent.set

---
## getAllProperties

**Signature:** `Array getAllProperties()`
**Return Type:** `Array`
**Call Scope:** unsafe
**Minimal Example:** `var props = {obj}.getAllProperties();`

**Description:**
Returns all active (non-deactivated) property IDs for this slider, including inherited ScriptComponent properties and ScriptSlider-specific properties.

---

## getChildComponents

**Signature:** `Array getChildComponents()`
**Return Type:** `Array`
**Call Scope:** unsafe
**Minimal Example:** `var children = {obj}.getChildComponents();`

**Description:**
Returns child ScriptComponent objects whose `parentComponent` references this component.

---

## getGlobalPositionX

**Signature:** `Integer getGlobalPositionX()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var x = {obj}.getGlobalPositionX();`

**Description:**
Returns the absolute x-position by summing this component's x offset and all parent offsets.

**Property Links:**
- Equivalent: none
- Related: get("x"), get("parentComponent")

---
## getGlobalPositionY

**Signature:** `Integer getGlobalPositionY()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var y = {obj}.getGlobalPositionY();`

**Description:**
Returns the absolute y-position by summing this component's y offset and all parent offsets.

**Property Links:**
- Equivalent: none
- Related: get("y"), get("parentComponent")

---
## getHeight

**Signature:** `Integer getHeight()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var h = {obj}.getHeight();`

**Description:**
Returns the slider height in pixels.

**Property Links:**
- Equivalent: get("height")
- Related: set("height", value), setPosition(...)

---
## getId

**Signature:** `String getId()`
**Return Type:** `String`
**Call Scope:** safe
**Minimal Example:** `var id = {obj}.getId();`

**Description:**
Returns the component ID used when creating the slider.

---

## getLocalBounds

**Signature:** `Array getLocalBounds(Double reduceAmount)`
**Return Type:** `Array`
**Call Scope:** safe
**Minimal Example:** `var b = {obj}.getLocalBounds(0);`

**Description:**
Returns local bounds as `[x, y, w, h]` reduced by `reduceAmount` on each edge.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| reduceAmount | Double | no | Amount to inset each edge | >= 0.0 |

**Property Links:**
- Equivalent: none
- Related: get("width"), get("height")

---
## getMaxValue

**Signature:** `Double getMaxValue()`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var max = {obj}.getMaxValue();`

**Description:**
Returns the current upper handle value for range sliders.

**Pitfalls:**
- Only meaningful in `Range` style. In other styles it logs an error and returns `1.0`.

**Property Links:**
- Equivalent: none
- Related: min/max range semantics (Range style helper)

**Cross References:**
- `$API.ScriptSlider.setMaxValue$`
- `$API.ScriptSlider.setStyle$`
- `$API.ScriptSlider.getMinValue$`
- `$API.ScriptSlider.contains$`

---

## getMinValue

**Signature:** `Double getMinValue()`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var min = {obj}.getMinValue();`

**Description:**
Returns the current lower handle value for range sliders.

**Pitfalls:**
- Only meaningful in `Range` style. In other styles it logs an error and returns `0.0`.

**Property Links:**
- Equivalent: none
- Related: min/max range semantics (Range style helper)

**Cross References:**
- `$API.ScriptSlider.setMinValue$`
- `$API.ScriptSlider.setStyle$`
- `$API.ScriptSlider.getMaxValue$`
- `$API.ScriptSlider.contains$`

---

## getValue

**Signature:** `var getValue()`
**Return Type:** `var`
**Call Scope:** safe
**Minimal Example:** `var v = {obj}.getValue();`

**Description:**
Returns the slider's current value. Uses a read lock for object-valued states.

**Pitfalls:**
- String values are not supported as component values.

**Cross References:**
- `$API.ScriptSlider.setValue$`
- `$API.ScriptSlider.changed$`

---

## getValueNormalized

**Signature:** `Double getValueNormalized()`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var n = {obj}.getValueNormalized();`

**Description:**
Converts the current slider value into normalized 0..1 space using `min`, `max`, `stepSize`, and an optional midpoint skew. Midpoint skew is only applied when `middlePosition` resolves to a numeric value strictly inside the current range. The explicit string sentinel `"disabled"` always bypasses skew.

**Pitfalls:**
- If range settings are invalid, it returns `0.0` (and logs details in backend builds).
- Legacy projects that used `-1` as an implicit disable token can now produce skew in ranges that include `-1`; use `"disabled"` for explicit no-skew behavior.

**Cross References:**
- `$API.ScriptSlider.setValueNormalized$`
- `$API.ScriptSlider.setRange$`
- `$API.ScriptSlider.setMidPoint$`

---

## getWidth

**Signature:** `Integer getWidth()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var w = {obj}.getWidth();`

**Description:**
Returns the slider width in pixels.

**Property Links:**
- Equivalent: get("width")
- Related: set("width", value), setPosition(...)

---
## grabFocus

**Signature:** `undefined grabFocus()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.grabFocus();`

**Description:**
Requests keyboard focus by notifying the first registered z-level listener.

**Cross References:**
- `$API.ScriptSlider.loseFocus$`

---

## loseFocus

**Signature:** `undefined loseFocus()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.loseFocus();`

**Description:**
Requests focus loss by notifying all registered z-level listeners.

**Cross References:**
- `$API.ScriptSlider.grabFocus$`

---

## sendRepaintMessage

**Signature:** `undefined sendRepaintMessage()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.sendRepaintMessage();`

**Description:**
Sends an asynchronous repaint message for this component.

---

## set

**Signature:** `undefined set(String propertyName, NotUndefined value)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.set("mode", "Decibel");`

**Description:**
Sets a property value on this slider. Invalid property names report a script error.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| propertyName | String | no | Property identifier to set | Must be a valid ScriptSlider property |
| value | NotUndefined | no | New property value | Must match expected property type |

**Property Links:**
- Equivalent: canonical property setter API (`set("<propertyId>", value)`)
- Related: ScriptComponent.get

**Cross References:**
- `$API.ScriptSlider.get$`
- `$API.ScriptSlider.setPropertiesFromJSON$`

---

## setConsumedKeyPresses

**Signature:** `undefined setConsumedKeyPresses(NotUndefined listOfKeys)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setConsumedKeyPresses("all");`

**Description:**
Defines which key presses this slider consumes. This must be called before `setKeyPressCallback` when using keyboard callbacks.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| listOfKeys | NotUndefined | no | Key description(s) as String, JSON object, or Array | Supports special values and KeyPress descriptors |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| "all" | Consume all key presses exclusively |
| "all_nonexclusive" | Consume all key presses but allow parent handling |

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| keyCode | int | JUCE key code (required for object form) |
| shift | bool | Shift modifier required |
| cmd / ctrl | bool | Cmd/Ctrl modifier required |
| alt | bool | Alt modifier required |
| character | String | Optional character |

**Pitfalls:**
- Invalid key descriptors report an error and are not registered.

**Cross References:**
- `$API.ScriptSlider.setKeyPressCallback$`

---

## setControlCallback

**Signature:** `undefined setControlCallback(Function controlFunction)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setControlCallback(onSliderControl);`

**Description:**
Sets a custom control callback for this slider.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| controlFunction | Function | yes | Inline callback `(component, value)` | Must be inline and take exactly 2 parameters |

**Callback Signature:** controlFunction(component: ScriptComponent, value: var)

**Pitfalls:**
- Callback must be an `inline function`.
- Callback must declare exactly 2 parameters.
- If the active scriptnode network forwards controls to parameters, this reports an error.

**Property Links:**
- Equivalent: none
- Related: processorId, parameterId

**Interaction Notes:**
- If `processorId` and `parameterId` are configured for processor forwarding, this custom callback path is bypassed.

**Cross References:**
- `$API.ScriptSlider.changed$`

**Example:**
```javascript:slider-control-callback
// Title: Custom inline control callback for a slider
const var Slider1 = Content.addKnob("Slider1", 0, 0);
reg lastControlValue = -1.0;
reg lastControlId = "";

inline function onSliderControl(component, value)
{
    lastControlId = component.getId();
    lastControlValue = value;
    Console.print(component.getId() + ": " + value);
}

Slider1.setControlCallback(onSliderControl);
```
```json:testMetadata:slider-control-callback
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "Slider1.setValue(0.25) || Slider1.changed()", "value": false},
    {"type": "REPL", "expression": "lastControlId", "value": "Slider1"},
    {"type": "REPL", "expression": "lastControlValue", "value": 0.25}
  ]
}
```

---

## setKeyPressCallback

**Signature:** `undefined setKeyPressCallback(Function keyboardFunction)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setKeyPressCallback(onSliderKeyPress);`

**Description:**
Registers a key/focus callback for this component.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| keyboardFunction | Function | yes | Inline callback receiving one event object | Call `setConsumedKeyPresses` first |

**Callback Signature:** keyboardFunction(event: Object)

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| isFocusChange | bool | True for focus-change events |
| hasFocus | bool | Present on focus-change events |
| character | String | Printable character for key events |
| specialKey | bool | True for non-printable keys |
| isWhitespace | bool | True if character is whitespace |
| isLetter | bool | True if character is alphabetic |
| isDigit | bool | True if character is numeric |
| keyCode | int | JUCE key code |
| description | String | Human-readable key description |
| shift | bool | Shift modifier state |
| cmd | bool | Cmd/Ctrl modifier state |
| alt | bool | Alt modifier state |

**Pitfalls:**
- If `setConsumedKeyPresses` was not called first, this reports an error.

**Cross References:**
- `$API.ScriptSlider.setConsumedKeyPresses$`

---

## setLocalLookAndFeel

**Signature:** `undefined setLocalLookAndFeel(ScriptObject lafObject)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setLocalLookAndFeel(laf);`

**Description:**
Assigns a local scripted look-and-feel object to this slider (and its child script components).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| lafObject | ScriptObject | no | ScriptedLookAndFeel object, or false/empty var to clear | Must be a ScriptedLookAndFeel object |

**Pitfalls:**
- Applies recursively to child script components.
- If the LAF uses CSS, class/style state is initialized automatically.

---

## setMaxValue

**Signature:** `undefined setMaxValue(Number max)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setMaxValue(0.75);`

**Description:**
Sets the upper range-handle value for range sliders and schedules an async UI update.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| max | Number | yes | New upper range value | Range style only |

**Pitfalls:**
- Only works in `Range` style. Other styles only log an error.

**Property Links:**
- Equivalent: none
- Related: min/max range semantics (Range style helper)

**Cross References:**
- `$API.ScriptSlider.getMaxValue$`
- `$API.ScriptSlider.setMinValue$`
- `$API.ScriptSlider.setStyle$`
- `$API.ScriptSlider.contains$`

---

## setMidPoint

**Signature:** `undefined setMidPoint(Colour valueForMidPoint)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setMidPoint("disabled");`

**Description:**
Sets the midpoint source used for skew mapping. Accepts a numeric midpoint, a numeric string, or the explicit sentinel string `"disabled"`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| valueForMidPoint | Colour | yes | Numeric midpoint (number or numeric string), or `"disabled"` | Non-numeric strings must be exactly `"disabled"` |

**Pitfalls:**
- `-1` is now treated as a normal numeric midpoint candidate; it only disables skew when it is outside the current range.
- For explicit no-skew behavior independent of range, pass `"disabled"`.

**Property Links:**
- Equivalent: none
- Related: set("middlePosition", value)

**Example:**
```javascript:slider-midpoint-numeric-and-disabled
// Title: Use numeric and explicit disabled midpoint routes
const var Slider1 = Content.addKnob("Slider1", 0, 0);

Slider1.setRange(-48.0, 0.0, 0.1);
Slider1.setMidPoint(-12.0);      // Numeric midpoint route
Slider1.setMidPoint("disabled"); // Explicit no-skew route

Slider1.setRange(0.0, 10.0, 0.1);
Slider1.setMidPoint("1.5");     // Numeric string route
```
```json:testMetadata:slider-midpoint-numeric-and-disabled
{
  "testable": true,
  "verifyType": "REPL",
  "verifyChecks": [
    {
      "expression": "Content.getComponent(\"Slider1\").get(\"middlePosition\")",
      "value": "1.5"
    }
  ]
}
```

**Cross References:**
- `$API.ScriptSlider.setRange$`
- `$API.ScriptSlider.setValueNormalized$`

---

## setMinValue

**Signature:** `undefined setMinValue(Number min)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setMinValue(0.25);`

**Description:**
Sets the lower range-handle value for range sliders and schedules an async UI update.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| min | Number | yes | New lower range value | Range style only |

**Pitfalls:**
- Only works in `Range` style. Other styles only log an error.

**Property Links:**
- Equivalent: none
- Related: min/max range semantics (Range style helper)

**Cross References:**
- `$API.ScriptSlider.getMinValue$`
- `$API.ScriptSlider.setMaxValue$`
- `$API.ScriptSlider.setStyle$`
- `$API.ScriptSlider.contains$`

---

## setMode

**Signature:** `undefined setMode(String mode)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setMode("Decibel");`

**Description:**
Sets the slider conversion/display mode (`Frequency`, `Decibel`, `Time`, `TempoSync`, `Linear`, `Discrete`, `Pan`, `NormalizedPercentage`). If the slider currently uses an untouched default range for the old mode, this also migrates range, step size, suffix, and midpoint defaults to the new mode.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| mode | String | yes | Target slider mode | Must match one of the valid mode strings |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| "Frequency" | Uses frequency-oriented range defaults and text conversion |
| "Decibel" | Uses dB-oriented range defaults and text conversion |
| "Time" | Uses time-oriented range defaults and text conversion |
| "TempoSync" | Uses tempo-index range and tempo-sync text conversion |
| "Linear" | Plain linear numeric mode |
| "Discrete" | Step-based discrete integer-like mode |
| "Pan" | Centered pan mode with left/right suffix behavior |
| "NormalizedPercentage" | 0..1 mode displayed as percentage |

**Pitfalls:**
- [BUG] Invalid mode strings silently switch internal runtime behavior to Linear without updating the `mode` property or reporting an error.

**Property Links:**
- Equivalent: none
- Related: set("mode", mode)

**Cross References:**
- `$API.ScriptSlider.setRange$`
- `$API.ScriptSlider.setValueNormalized$`
- `$API.ScriptSlider.getValueNormalized$`
- `$API.ScriptSlider.setStyle$`

---

## setModifiers

**Signature:** `undefined setModifiers(String action, IndexOrArray modifiers)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setModifiers("FineTune", [mods.shiftDown]);`

**Description:**
Stores modifier mappings for slider interaction actions. Use constants from `createModifiers()` for action names and modifier flags.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| action | String | yes | Action name to configure | `TextInput`, `FineTune`, `ResetToDefault`, `ContextMenu`, `ScaleModulation` |
| modifiers | IndexOrArray | yes | Modifier data (single flag or array of flags) | Use values from `createModifiers()` |

**Cross References:**
- `$API.ScriptSlider.createModifiers$`

**Example:**
```javascript:slider-modifier-mapping
// Title: Configure modifier mappings with the Modifiers object
const var Slider1 = Content.addKnob("Slider1", 0, 0);
const var mods = Slider1.createModifiers();

Slider1.setModifiers(mods.FineTune, [mods.shiftDown]);
Slider1.setModifiers(mods.ResetToDefault, [mods.doubleClick]);
```
```json:testMetadata:slider-modifier-mapping
{
  "testable": false,
  "skipReason": "Modifier mappings affect mouse interaction paths that are not script-triggerable in the validator"
}
```

---

## setPosition

**Signature:** `undefined setPosition(Integer x, Integer y, Integer w, Integer h)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setPosition(10, 10, 128, 48);`

**Description:**
Sets x, y, width, and height in one call.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| x | Integer | no | X position | 0-900 |
| y | Integer | no | Y position | 0-MAX_SCRIPT_HEIGHT |
| w | Integer | no | Width | 0-900 |
| h | Integer | no | Height | 0-MAX_SCRIPT_HEIGHT |

**Property Links:**
- Equivalent: none
- Related: set("x", x), set("y", y), set("width", w), set("height", h)

---
## setPropertiesFromJSON

**Signature:** `undefined setPropertiesFromJSON(JSON jsonData)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setPropertiesFromJSON({"mode": "Linear", "stepSize": 0.01});`

**Description:**
Sets multiple properties at once from a JSON object. Properties in `priorityProperties` are applied first (`mode` for ScriptSlider), then remaining properties are applied in property-list order.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| jsonData | JSON | no | Property map to apply | Keys must be valid ScriptSlider property names |

---

## setRange

**Signature:** `undefined setRange(Number min, Number max, Number stepSize)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setRange(20.0, 20000.0, 1.0);`

**Description:**
Sets slider min/max/step size in one call.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| min | Number | no | Lower range bound | Must be < max for usable range |
| max | Number | no | Upper range bound | Must be > min for usable range |
| stepSize | Number | no | Step interval | Must be >= 0.0 |

**Pitfalls:**
- Invalid ranges (min >= max, negative step, extreme limits) disable the runtime slider widget until values are fixed.

**Property Links:**
- Equivalent: none
- Related: set("min", min), set("max", max), set("stepSize", stepSize)

**Cross References:**
- `$API.ScriptSlider.setMidPoint$`
- `$API.ScriptSlider.setValueNormalized$`
- `$API.ScriptSlider.getValueNormalized$`
- `$API.ScriptSlider.setMode$`

---

## setStyle

**Signature:** `undefined setStyle(String style)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setStyle("Knob");`

**Description:**
Sets slider style (`Knob`, `Horizontal`, `Vertical`, `Range`).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| style | String | yes | Target style | Must match one of the valid style names |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| "Knob" | Rotary slider style |
| "Horizontal" | Horizontal linear bar style |
| "Vertical" | Vertical linear bar style |
| "Range" | Two-value horizontal range style |

**Pitfalls:**
- [BUG] Invalid style strings are stored in the property tree but do not update `styleId`, so runtime behavior may not match the stored property text.

**Property Links:**
- Equivalent: none
- Related: set("style", style)

**Cross References:**
- `$API.ScriptSlider.contains$`
- `$API.ScriptSlider.setMinValue$`
- `$API.ScriptSlider.setMaxValue$`
- `$API.ScriptSlider.getMinValue$`
- `$API.ScriptSlider.getMaxValue$`
- `$API.ScriptSlider.setMode$`

---

## setStyleSheetClass

**Signature:** `undefined setStyleSheetClass(String classIds)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setStyleSheetClass(".large .accent");`

**Description:**
Sets CSS class selectors for this component. The component type class is prepended automatically.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| classIds | String | no | Space-separated class selector list | CSS selector tokens |

---

## setStyleSheetProperty

**Signature:** `undefined setStyleSheetProperty(String variableId, NotUndefined value, String type)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setStyleSheetProperty("trackColor", Colours.red, "color");`

**Description:**
Sets a CSS variable value for this component, with optional conversion by `type`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| variableId | String | no | CSS variable identifier | Any valid variable key |
| value | NotUndefined | no | Variable value | Must match selected conversion |
| type | String | no | Conversion mode | See value descriptions |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| "path" | Convert Path object to base64 path data |
| "color" | Convert colour to CSS `#AARRGGBB` |
| "%" | Convert number to percent string |
| "px" | Convert number to pixel string |
| "em" | Convert number to em string |
| "vh" | Convert number to viewport-height string |
| "deg" | Convert number to degree string |
| "" | Store raw value |

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
| pseudoState | String | no | Pseudo-state selector string | Use valid pseudo-state tokens or empty string |

---

## setTooltip

**Signature:** `undefined setTooltip(String tooltip)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setTooltip("Drive: {VALUE} dB");`

**Description:**
Sets tooltip text.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| tooltip | String | no | Tooltip text | Any string |

**Property Links:**
- Equivalent: set("tooltip", tooltip)
- Related: get("tooltip")

---
## setValue

**Signature:** `undefined setValue(NotUndefined newValue)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setValue(0.5);`

**Description:**
Sets the slider value, schedules async UI update, and broadcasts value listener notifications.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newValue | NotUndefined | no | New slider value | Must not be String |

**Pitfalls:**
- String values report a script error.
- Values set during `onInit` are not restored on recompilation (`skipRestoring = true`).

**Property Links:**
- Equivalent: none
- Related: linkedTo

**Interaction Notes:**
- Value propagation can forward to linked components through the `linkedTo` routing setup.

**Cross References:**
- `$API.ScriptSlider.getValue$`
- `$API.ScriptSlider.changed$`
- `$API.ScriptSlider.setValueWithUndo$`

---

## setValueNormalized

**Signature:** `undefined setValueNormalized(Double normalizedValue)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setValueNormalized(0.5);`

**Description:**
Maps normalized 0..1 input to actual slider value using current range and optional midpoint skew, then calls `setValue`. Midpoint skew is only applied when `middlePosition` resolves to a numeric value strictly inside the active range. The `"disabled"` sentinel bypasses skew.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| normalizedValue | Double | no | Normalized input value | Typically 0.0..1.0 |

**Pitfalls:**
- With invalid range settings, it does not update the value (backend builds log details).
- Legacy projects that used `-1` as an implicit disable token can get skew when range includes `-1`; use `setMidPoint("disabled")` for explicit no-skew behavior.

**Cross References:**
- `$API.ScriptSlider.getValueNormalized$`
- `$API.ScriptSlider.setRange$`
- `$API.ScriptSlider.setMidPoint$`

---

## setValuePopupFunction

**Signature:** `undefined setValuePopupFunction(Function newFunction)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setValuePopupFunction(onPopupText);`

**Description:**
Sets a callback used to format popup text shown during slider drags when value popups are enabled.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newFunction | Function | yes | Popup text callback receiving current numeric value | Must accept exactly 1 argument |

**Callback Signature:** newFunction(value: double)

**Example:**
```javascript:slider-popup-format-function
// Title: Custom popup text formatting
const var Slider1 = Content.addKnob("Slider1", 0, 0);
Slider1.set("showValuePopup", "Above");

inline function onPopupText(value)
{
    return "Drive: " + value + " dB";
}

Slider1.setValuePopupFunction(onPopupText);
```
```json:testMetadata:slider-popup-format-function
{
  "testable": false,
  "skipReason": "Popup rendering text is only visible during drag UI interaction and cannot be asserted via script state"
}
```

---

## setValueWithUndo

**Signature:** `undefined setValueWithUndo(NotUndefined newValue)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setValueWithUndo(0.5);`

**Description:**
Sets value through the undo manager by creating an undoable control event.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newValue | NotUndefined | no | New value to set with undo support | Must not be String |

**Pitfalls:**
- Do not call from `onControl` callbacks.

**Property Links:**
- Equivalent: none
- Related: useUndoManager

**Interaction Notes:**
- Undo integration depends on `useUndoManager`; if disabled, undo history integration is not active.

**Cross References:**
- `$API.ScriptSlider.setValue$`
- `$API.ScriptSlider.changed$`

---

## setZLevel

**Signature:** `undefined setZLevel(String zLevel)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setZLevel("Front");`

**Description:**
Sets component z-level ordering (`Back`, `Default`, `Front`, `AlwaysOnTop`) and notifies z-level listeners if changed.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| zLevel | String | no | Target z-level name | Must match valid z-level string |

---

## showControl

**Signature:** `undefined showControl(Integer shouldBeVisible)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.showControl(1);`

**Description:**
Shows or hides the component by updating its `visible` property.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shouldBeVisible | Integer | no | Visibility state | 1 = show, 0 = hide |

**Property Links:**
- Equivalent: none
- Related: set("visible", shouldBeVisible), get("visible")

**Cross References:**
- `$API.ScriptSlider.fadeComponent$`

---

## updateValueFromProcessorConnection

**Signature:** `undefined updateValueFromProcessorConnection()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.updateValueFromProcessorConnection();`

**Description:**
Refreshes slider value from its processor/parameter connection if one is configured.

Special parameter indices:
- `-2` modulation intensity
- `-3` bypass as 1.0/0.0
- `-4` inverted bypass as 0.0/1.0

If no connection is active, the method does nothing.

**Property Links:**
- Equivalent: none
- Related: get("processorId"), get("parameterId"), setValue(...)

**Cross References:**
- `$API.ScriptSlider.connectToModulatedParameter$`
