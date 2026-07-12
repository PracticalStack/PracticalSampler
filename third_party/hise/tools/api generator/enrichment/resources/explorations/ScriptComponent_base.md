# ScriptComponent Base Class -- Complete Public API Reference

## Source Files

- **Header:** `hi_scripting/scripting/api/ScriptingApiContent.h` (lines 211-962)
- **Implementation:** `hi_scripting/scripting/api/ScriptingApiContent.cpp`
- **Class:** `ScriptingApi::Content::ScriptComponent` (aliased as `hise::ScriptComponent` at line 3588 of the header)
- **Inheritance:** `RestorableObject`, `ConstScriptingObject`, `AssignableObject`, `SafeChangeBroadcaster`, `UpdateDispatcher::Listener`

## Child Classes That Inherit ScriptComponent

All of the following inherit from `ScriptComponent` (directly or via `ComplexDataScriptComponent`):

| Child Class | Intermediary | Additional Methods? |
|---|---|---|
| `ScriptSlider` | direct | Yes (mode, range, etc.) |
| `ScriptButton` | direct | Yes (filmstrip, etc.) |
| `ScriptComboBox` | direct | Yes (items, etc.) |
| `ScriptLabel` | direct | Yes (font, editable, etc.) |
| `ScriptTable` | `ComplexDataScriptComponent` | Yes |
| `ScriptSliderPack` | `ComplexDataScriptComponent` | Yes |
| `ScriptAudioWaveform` | `ComplexDataScriptComponent` | Yes |
| `ScriptImage` | direct | Yes |
| `ScriptPanel` | direct | Yes (many: paint, animation, etc.) |
| `ScriptedViewport` | direct | Yes |
| `ScriptWebView` | direct | Yes |
| `ScriptFloatingTile` | direct | Yes |
| `ScriptDynamicContainer` | direct | Yes |
| `ScriptMultipageDialog` | direct | Yes |

---

## Registered Scripting API Methods (35 total)

These are the methods registered via `ADD_API_METHOD_*` in the ScriptComponent constructor (lines 421-454 of ScriptingApiContent.cpp) and defined in the `Wrapper` struct (lines 219-255). Every child component class inherits all of these.

---

### 1. `get`

```cpp
var get(String propertyName) const;
```

**Parameters:**
- `propertyName` (`String`) -- The name of a component property to retrieve (e.g., `"text"`, `"visible"`, `"bgColour"`, `"x"`, `"width"`, etc.).

**Returns:** The current value of the property. If the property is set on the component's value tree, returns that value; otherwise returns the default. Reports a script error if the property does not exist.

**Base properties available on all components:** `text`, `visible`, `enabled`, `locked`, `x`, `y`, `width`, `height`, `min`, `max`, `defaultValue`, `tooltip`, `bgColour`, `itemColour`, `itemColour2`, `textColour`, `macroControl`, `saveInPreset`, `isPluginParameter`, `pluginParameterName`, `pluginParameterGroup`, `deferControlCallback`, `isMetaParameter`, `linkedTo`, `automationId`, `useUndoManager`, `parentComponent`, `processorId`, `parameterId`.

Child component types add additional properties (e.g., `ScriptSlider` adds `Mode`, `Style`, `stepSize`, etc.).

**Behavioral notes:** None. Safe to call anytime.

---

### 2. `set`

```cpp
void set(String propertyName, var value);
```

**Parameters:**
- `propertyName` (`String`) -- The property identifier to set. Must be a valid property ID for this component type.
- `value` (`var`) -- The new value.

**Behavioral notes:** Reports a script error if the property does not exist. During `onInit`, changes are applied without notification; outside `onInit`, sends change notifications to update the UI. Tracks which properties have been set by script for profiling purposes.

---

### 3. `getId`

```cpp
String getId() const;
```

**Parameters:** None.

**Returns:** The component's ID as a string (the variable name used when creating the component, e.g., `"Knob1"`).

---

### 4. `getValue`

```cpp
virtual var getValue() const;
```

**Parameters:** None.

**Returns:** The current value of the component. Uses a `SimpleReadWriteLock` for thread-safe read access. Asserts that the value is not a string (strings must not be stored as values).

**Override behavior:** Overridden by `ScriptLabel` (returns text content), `ScriptSliderPack` (returns pack data), `ScriptFloatingTile` (returns floating tile data), `ScriptedViewport` (returns viewport data), `ScriptMultipageDialog` (returns `var()`).

---

### 5. `setValue`

```cpp
virtual void setValue(var newValue);
```

**Parameters:**
- `newValue` (`var`) -- The new value to set. Must NOT be a String (reports script error). Can be a number, boolean, or object.

**Behavioral notes:**
- Thread-safe: can be called from message callbacks; the UI update happens asynchronously.
- If called during `onInit`, the value will NOT be restored after recompilation (`skipRestoring` is set to true).
- Propagates the value to all linked component targets.
- Triggers an async UI update via `triggerAsyncUpdate()`.
- Sends value listener messages.

**Override behavior:** Overridden by `ScriptSlider` (clamps to range), `ScriptLabel`, `ScriptSliderPack`, `ScriptedViewport`, `ScriptFloatingTile`, `ScriptMultipageDialog` (no-op).

---

### 6. `setValueNormalized`

```cpp
virtual void setValueNormalized(double normalizedValue);
```

**Parameters:**
- `normalizedValue` (`double`) -- A value in the range 0.0 to 1.0.

**Behavioral notes:** The base implementation simply calls `setValue(normalizedValue)`. The behavior changes significantly for `ScriptSlider`, which maps the normalized 0..1 range to the slider's actual min/max range using its configured mode (linear, frequency, decibel, etc.).

**Override behavior:** Overridden by `ScriptSlider`.

---

### 7. `setValueWithUndo`

```cpp
void setValueWithUndo(var newValue);
```

**Parameters:**
- `newValue` (`var`) -- The new value to set with undo support.

**Behavioral notes:** Creates an `UndoableControlEvent` and performs it through the undo manager. Do NOT call this from `onControl` callbacks -- it is intended for user-initiated value changes that should be undoable.

---

### 8. `getValueNormalized`

```cpp
virtual double getValueNormalized() const;
```

**Parameters:** None.

**Returns:** The normalized value (0.0 to 1.0). Base implementation returns `getValue()` directly.

**Override behavior:** Overridden by `ScriptSlider` to map the actual value back to the 0..1 range.

---

### 9. `setColour`

```cpp
void setColour(int colourId, int colourAs32bitHex);
```

**Parameters:**
- `colourId` (`int`) -- Which colour to set:
  - `0` = `bgColour` (background colour)
  - `1` = `itemColour` (first item colour)
  - `2` = `itemColour2` (second item colour)
  - `3` = `textColour` (text colour)
- `colourAs32bitHex` (`int`) -- The colour as a 32-bit ARGB hex value (e.g., `0xFFFF0000` for red).

---

### 10. `setPosition`

```cpp
void setPosition(int x, int y, int w, int h);
```

**Parameters:**
- `x` (`int`) -- X position in pixels, relative to parent.
- `y` (`int`) -- Y position in pixels, relative to parent.
- `w` (`int`) -- Width in pixels.
- `h` (`int`) -- Height in pixels.

**Behavioral notes:** Directly sets the `x`, `y`, `width`, `height` properties on the property tree.

---

### 11. `setTooltip`

```cpp
void setTooltip(const String& tooltip);
```

**Parameters:**
- `tooltip` (`String`) -- The tooltip text to display on mouse hover.

---

### 12. `showControl`

```cpp
void showControl(bool shouldBeVisible);
```

**Parameters:**
- `shouldBeVisible` (`bool`) -- `true` to show, `false` to hide the component.

**Behavioral notes:** Sets the `visible` property with change message notification.

---

### 13. `addToMacroControl`

```cpp
void addToMacroControl(int macroIndex);
```

**Parameters:**
- `macroIndex` (`int`) -- The macro controller index (0 to 7). Sets the internal `connectedMacroIndex`.

---

### 14. `getWidth`

```cpp
var getWidth() const;
```

**Returns:** The `width` property as an integer.

---

### 15. `getHeight`

```cpp
var getHeight() const;
```

**Returns:** The `height` property as an integer.

---

### 16. `getLocalBounds`

```cpp
var getLocalBounds(float reduceAmount);
```

**Parameters:**
- `reduceAmount` (`float`) -- The amount in pixels to inset from each edge.

**Returns:** An array `[x, y, w, h]` (or a rectangle object if `HISE_USE_SCRIPT_RECTANGLE_OBJECT` is enabled) representing the local bounds reduced by the given amount. The local bounds start at `[0, 0, width, height]`.

---

### 17. `getChildComponents`

```cpp
var getChildComponents();
```

**Parameters:** None.

**Returns:** An array of `ScriptComponent` references for all child components (components whose `parentComponent` is set to this component). Does not include `this` in the result.

---

### 18. `changed`

```cpp
virtual void changed();
```

**Parameters:** None.

**Behavioral notes:**
- Triggers the control callback (either the custom one set via `setControlCallback` or the default `onControl` callback).
- **Cannot be called during `onInit`** -- if called during `onInit`, it logs a console message and returns without executing.
- If `deferControlCallback` is set, the callback is deferred to the message thread.
- If an error occurs during the callback execution and flaky threading is not allowed, a script error is reported to abort execution.
- After the callback, checks if an error occurred and re-throws to abort script execution if needed.

**Override behavior:** Overridden by `ScriptSliderPack` and `ScriptPanel`.

---

### 19. `getGlobalPositionX`

```cpp
int getGlobalPositionX();
```

**Returns:** The absolute x-position relative to the interface root, computed by recursively adding parent component x-offsets.

---

### 20. `getGlobalPositionY`

```cpp
int getGlobalPositionY();
```

**Returns:** The absolute y-position relative to the interface root, computed by recursively adding parent component y-offsets.

---

### 21. `setControlCallback`

```cpp
void setControlCallback(var controlFunction);
```

**Parameters:**
- `controlFunction` (`var`) -- An inline function with exactly 2 parameters: `function(component, value)`. Pass `undefined` to remove a custom callback.

**Behavioral notes:**
- The function must be an inline function (not a regular function reference). Reports a script error otherwise.
- Must have exactly 2 parameters. Reports a script error if the parameter count is wrong.
- If the script processor has a DspNetwork that is forwarding controls to parameters, setting a control callback reports an error.
- Passing `undefined` or empty `var()` clears the custom callback, reverting to the default `onControl` callback.

---

### 22. `getAllProperties`

```cpp
var getAllProperties();
```

**Parameters:** None.

**Returns:** An array of strings containing all active (non-deactivated) property IDs for this component. Includes both base ScriptComponent properties and any child-class-specific properties.

---

### 23. `setKeyPressCallback`

```cpp
void setKeyPressCallback(var keyboardFunction);
```

**Parameters:**
- `keyboardFunction` (`var`) -- An inline function with 1 parameter: `function(event)`. The `event` object has properties described below.

**Behavioral notes:**
- **MUST call `setConsumedKeyPresses()` BEFORE calling this method.** Reports a script error if `setConsumedKeyPresses` has not been called yet.
- The callback receives an event object with two possible shapes:

  **Key press event:**
  | Property | Type | Description |
  |---|---|---|
  | `isFocusChange` | `bool` | Always `false` for key events |
  | `character` | `String` | The printable character, or `""` for non-printable keys |
  | `specialKey` | `bool` | `true` if not a printable character |
  | `isWhitespace` | `bool` | `true` if the character is whitespace |
  | `isLetter` | `bool` | `true` if the character is a letter |
  | `isDigit` | `bool` | `true` if the character is a digit |
  | `keyCode` | `int` | The JUCE key code |
  | `description` | `String` | Human-readable description of the key press |
  | `shift` | `bool` | `true` if Shift is held |
  | `cmd` | `bool` | `true` if Cmd/Ctrl is held |
  | `alt` | `bool` | `true` if Alt is held |

  **Focus change event:**
  | Property | Type | Description |
  |---|---|---|
  | `isFocusChange` | `bool` | Always `true` for focus events |
  | `hasFocus` | `bool` | `true` if the component gained focus, `false` if it lost focus |

---

### 24. `setConsumedKeyPresses`

```cpp
void setConsumedKeyPresses(var listOfKeys);
```

**Parameters:**
- `listOfKeys` (`var`) -- One of the following formats:

**String enum values:**
| Value | Description |
|---|---|
| `"all"` | Catch all key presses exclusively (prevents parent from receiving them) |
| `"all_nonexclusive"` | Catch all key presses non-exclusively (parent still receives them) |

**Single key description** (`String`): A JUCE key press description string (e.g., `"A"`, `"ctrl + S"`, `"F5"`, `"shift + tab"`). Uses JUCE's `KeyPress::createFromDescription()`.

**Single key description** (`Object`/JSON): A JSON object with:
| Property | Type | Description |
|---|---|---|
| `keyCode` | `int` | The JUCE key code (required, must be non-zero) |
| `shift` | `bool` | Whether Shift modifier is required |
| `cmd` or `ctrl` | `bool` | Whether Cmd/Ctrl modifier is required (either key name works) |
| `alt` | `bool` | Whether Alt modifier is required |
| `character` | `String` | Optional character for the key press |

**Array of key descriptions** (`Array`): An array where each element is either a string or JSON object as described above.

**Behavioral notes:** Must be called BEFORE `setKeyPressCallback`. Reports a script error if an invalid key description is provided.

---

### 25. `loseFocus`

```cpp
void loseFocus();
```

**Parameters:** None.

**Behavioral notes:** Notifies all z-level listeners that the component wants to lose keyboard focus. Triggers the `wantsToLoseFocus()` callback on all registered `ZLevelListener` instances.

---

### 26. `grabFocus`

```cpp
void grabFocus();
```

**Parameters:** None.

**Behavioral notes:** Notifies z-level listeners that the component wants to grab keyboard focus. Only notifies the first listener (exclusive operation).

---

### 27. `setZLevel`

```cpp
void setZLevel(String zLevel);
```

**Parameters:**
- `zLevel` (`String`) -- The depth level for this component among its siblings.

**String enum values:**
| Value | Description |
|---|---|
| `"Back"` | Renders behind all sibling components |
| `"Default"` | Normal rendering order |
| `"Front"` | Renders in front of normal siblings |
| `"AlwaysOnTop"` | Always renders on top of all siblings |

**Behavioral notes:** Reports a script error if the value is not one of the four valid strings (case-sensitive). Notifies all z-level listeners when the level changes.

---

### 28. `setLocalLookAndFeel`

```cpp
void setLocalLookAndFeel(var lafObject);
```

**Parameters:**
- `lafObject` (`var`) -- A `ScriptedLookAndFeel` object, or `undefined`/empty to remove the custom look and feel.

**Behavioral notes:**
- The object must be a `ScriptedLookAndFeel` instance. If the object is not a valid LAF, the local look and feel is cleared.
- Registers the LAF with the content's LAF registry.
- If the LAF uses CSS (has a stylesheet), automatically calls `setStyleSheetClass({})` to initialize the class selector.
- **Propagates to all child components**: iterates over all child `ScriptComponent` instances and sets their `localLookAndFeel` to the same object.
- When CSS mode is active, color properties (`bgColour`, `itemColour`, `itemColour2`, `textColour`) are initialized in the property tree if not already present, and default-property-removal is disabled.

---

### 29. `sendRepaintMessage`

```cpp
virtual void sendRepaintMessage();
```

**Parameters:** None.

**Behavioral notes:** Sends an asynchronous repaint message via `repaintBroadcaster`. This is useful when you've changed visual properties programmatically and need to force a UI redraw.

**Override behavior:** Overridden by `ScriptPanel` (which calls its own `repaint()`) and `ScriptDynamicContainer`.

---

### 30. `fadeComponent`

```cpp
void fadeComponent(bool shouldBeVisible, int milliseconds);
```

**Parameters:**
- `shouldBeVisible` (`bool`) -- Target visibility state.
- `milliseconds` (`int`) -- Duration of the fade animation in milliseconds.

**Behavioral notes:** Only triggers if the target visibility differs from the current visibility. Sets the `visible` property and sends an async fade message through the global UI animator. The actual fade animation is handled by the UI component wrapper.

---

### 31. `setStyleSheetProperty`

```cpp
void setStyleSheetProperty(const String& variableId, const var& value, const String& type);
```

**Parameters:**
- `variableId` (`String`) -- The CSS variable name to set (used as a property key in the `ComponentStyleSheetProperties` value tree). This becomes available as a CSS variable in stylesheets.
- `value` (`var`) -- The value to assign.
- `type` (`String`) -- The type conversion to apply to the value before storing it.

**String enum values for `type`** (defined in `ApiHelpers::convertStyleSheetProperty`, `ScriptingApi.cpp` line 174):

| Value | Description | Conversion |
|---|---|---|
| `"path"` | Converts a Path object to a base64-encoded string | Calls `PathObject::toBase64()` |
| `"color"` | Converts a colour value (int) to a CSS color string | Outputs `"#AARRGGBB"` format |
| `"%"` | Converts a number to a CSS percentage string | Multiplies by 100, appends `"%"` (e.g., `0.5` becomes `"50%"`) |
| `"px"` | Converts a number to a CSS pixel value string | Appends `"px"` (e.g., `10` becomes `"10px"`) |
| `"em"` | Converts a number to a CSS em value string | Appends `"em"` |
| `"vh"` | Converts a number to a CSS viewport-height string | Appends `"vh"` |
| `"deg"` | Converts a number to a CSS degree string | Appends `"deg"` |
| `""` (empty string) or any other value | No conversion | Stores the value as-is |

**Behavioral notes:** Creates the `ComponentStyleSheetProperties` value tree if it does not yet exist. The variable is then available for CSS selectors that reference this component.

---

### 32. `setStyleSheetClass`

```cpp
void setStyleSheetClass(const String& classIds);
```

**Parameters:**
- `classIds` (`String`) -- A space-separated string of CSS class selectors to apply to this component (e.g., `".myClass .highlighted"`). The component's own type class (derived from the `type` property, lowercased, prefixed with `.`) is automatically prepended.

**Behavioral notes:** Creates the `ComponentStyleSheetProperties` value tree if it does not yet exist. Sets the `"class"` property. The component type class (e.g., `.scriptslider`, `.scriptbutton`) is always included as the first class.

---

### 33. `setStyleSheetPseudoState`

```cpp
void setStyleSheetPseudoState(const String& pseudoState);
```

**Parameters:**
- `pseudoState` (`String`) -- A string containing one or more CSS pseudo-state selectors. Multiple can be combined in one string.

**Valid pseudo-state strings** (parsed by `simple_css::PseudoState::getPseudoClassIndex`, `HelperClasses.cpp` line 65):

| Value | Description | Bitmask |
|---|---|---|
| `:first-child` | First child pseudo-class | 1 |
| `:last-child` | Last child pseudo-class | 2 |
| `:root` | Root element pseudo-class | 4 |
| `:hover` | Mouse hover state | 8 |
| `:active` | Active/pressed state | 16 |
| `:focus` | Keyboard focus state | 32 |
| `:disabled` | Disabled state | 64 |
| `:hidden` | Hidden state | 128 |
| `:checked` | Checked/toggled state | 256 |

Multiple states can be combined: e.g., `":hover:active"`, `":checked:focus"`.

Pass an empty string `""` to clear all pseudo-states (results in state `0` = `None`).

**Behavioral notes:** After setting the pseudo-state, automatically calls `sendRepaintMessage()` to trigger a visual update.

---

### 34. `updateValueFromProcessorConnection`

```cpp
void updateValueFromProcessorConnection();
```

**Parameters:** None.

**Behavioral notes:** Reads the current attribute value from the connected processor (set via the `processorId` and `parameterId` properties) and calls `setValue()` with that value. Special parameter index values:
- `-2`: Reads modulation intensity from a `Modulation` processor.
- `-3`: Reads bypass state (1.0 if bypassed, 0.0 if not).
- `-4`: Reads inverted bypass state (0.0 if bypassed, 1.0 if not).
- `>= 0`: Reads the attribute at the given parameter index.

Does nothing if no processor connection is established.

---

### 35. `setPropertiesFromJSON`

**NOTE:** This method is declared in the header under the `// API Methods` section (line 565), but it is **NOT registered** via `ADD_API_METHOD` in the constructor. It is therefore **not directly callable from HISEScript** on the component object. It is instead exposed on the `Content` object as `Content.setPropertiesFromJSON(componentName, jsonData)`.

---

## Virtual Methods -- Override Summary

The following table shows which of the 35 scripting API methods are `virtual` and which child classes override them:

| Method | Virtual? | Overridden By |
|---|---|---|
| `getValue` | Yes | `ScriptLabel`, `ScriptSliderPack`, `ScriptFloatingTile`, `ScriptedViewport`, `ScriptMultipageDialog` |
| `setValue` | Yes | `ScriptSlider`, `ScriptLabel`, `ScriptSliderPack`, `ScriptedViewport`, `ScriptFloatingTile`, `ScriptMultipageDialog` |
| `setValueNormalized` | Yes | `ScriptSlider` |
| `getValueNormalized` | Yes | `ScriptSlider` |
| `changed` | Yes | `ScriptSliderPack`, `ScriptPanel` |
| `sendRepaintMessage` | Yes | `ScriptPanel`, `ScriptDynamicContainer` |
| All other 29 methods | No | None (universally inherited, behavior is identical across all child types) |

---

## Base Properties (set in constructor, lines 394-419)

All ScriptComponent subclasses inherit these properties:

| Property ID | Default Value | Type |
|---|---|---|
| `text` | Component name | String |
| `visible` | `true` | Toggle |
| `enabled` | `true` | Toggle |
| `locked` | `false` | Toggle |
| `x` | (set by creation) | Number (0-900) |
| `y` | (set by creation) | Number (0-MAX_SCRIPT_HEIGHT) |
| `width` | (set by creation) | Number (0-900) |
| `height` | (set by creation) | Number (0-MAX_SCRIPT_HEIGHT) |
| `min` | `0.0` | Number |
| `max` | `1.0` | Number |
| `defaultValue` | `0` | Number |
| `tooltip` | `""` | String |
| `bgColour` | `0x55FFFFFF` | Colour |
| `itemColour` | `0x66333333` | Colour |
| `itemColour2` | `0xFB111111` | Colour |
| `textColour` | `0xFFFFFFFF` | Colour |
| `macroControl` | `-1` | Choice (0-7 or -1) |
| `saveInPreset` | `true` | Toggle |
| `isPluginParameter` | `false` | Toggle (deactivated by default) |
| `pluginParameterName` | `""` | String |
| `pluginParameterGroup` | `""` | Choice |
| `deferControlCallback` | `false` | Toggle |
| `isMetaParameter` | `false` | Toggle |
| `linkedTo` | `""` | Choice |
| `automationId` | `""` | Choice |
| `useUndoManager` | `false` | Toggle |
| `parentComponent` | `""` | Choice |
| `processorId` | `" "` (space) | Choice |
| `parameterId` | `""` | Choice |

---

## Key Behavioral Constraints Summary

| Method | Constraint |
|---|---|
| `setValue` | Do NOT pass strings. Safe from any thread. Value set in `onInit` will not be restored after recompilation. |
| `setValueWithUndo` | Do NOT call from `onControl` callbacks. |
| `changed` | Cannot be called during `onInit` (silently skipped with console message). |
| `setKeyPressCallback` | Must call `setConsumedKeyPresses` first, or a script error is thrown. |
| `setConsumedKeyPresses` | Must be called before `setKeyPressCallback`. |
| `setControlCallback` | Function must be an inline function with exactly 2 parameters. Will error if a DspNetwork is forwarding controls. |
| `setZLevel` | String must be exactly one of the four valid values (case-sensitive). |
| `setLocalLookAndFeel` | Propagates to all child components. Initializes CSS class selectors if the LAF uses CSS. |
| `setStyleSheetPseudoState` | Triggers repaint automatically. |
| `setStyleSheetProperty` | The `type` parameter determines the CSS unit conversion. |
