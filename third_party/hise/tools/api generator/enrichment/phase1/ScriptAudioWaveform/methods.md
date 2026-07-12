# ScriptAudioWaveform -- Method Documentation

---

## getRangeStart

**Signature:** `Integer getRangeStart()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var start = {obj}.getRangeStart();`

**Description:**
Returns the start position (in samples) of the currently selected sample range on the waveform display. If no audio data is loaded or the cached audio file reference is invalid, returns `0`. The range is typically set by user interaction with the draggable SampleArea edges or programmatically via the underlying MultiChannelAudioBuffer.

**Property Links:**
- Equivalent: none
- Related: enableRange

**Cross References:**
- `$API.ScriptAudioWaveform.getRangeEnd$`

---

## getRangeEnd

**Signature:** `Integer getRangeEnd()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var end = {obj}.getRangeEnd();`

**Description:**
Returns the end position (in samples) of the currently selected sample range on the waveform display. If no audio data is loaded or the cached audio file reference is invalid, returns `0`. The range is typically set by user interaction with the draggable SampleArea edges or programmatically via the underlying MultiChannelAudioBuffer.

**Property Links:**
- Equivalent: none
- Related: enableRange

**Cross References:**
- `$API.ScriptAudioWaveform.getRangeStart$`

---

## referToData

**Signature:** `undefined referToData(ScriptObject audioData)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.referToData(myAudioFile);`

**Description:**
Connects this waveform component to an external audio data source. After calling this method, the waveform displays the audio data from the referenced source instead of its own internal buffer. Accepts three argument types:

- A `ScriptAudioFile` object (from `Engine.createAndRegisterAudioFile()` or `registerAtParent()`) -- connects to the audio file handle's data
- Another `ScriptAudioWaveform` component -- shares the same data source as that component
- The integer `-1` -- resets to the component's own internal audio buffer

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| audioData | ScriptObject | no | A ScriptAudioFile handle, another ScriptAudioWaveform, or -1 to reset | Must be a valid data reference or -1 |

**Pitfalls:**
- [BUG] Passing an object that is neither a ScriptAudioFile, a ComplexDataScriptComponent, nor `-1` silently does nothing -- no error is reported and the data source is unchanged.

**Property Links:**
- Equivalent: none
- Related: processorId, sampleIndex source binding

**Cross References:**
- `$API.ScriptAudioWaveform.registerAtParent$`

**Example:**
```javascript:referToData-audio-file
// Title: Connect waveform to a ScriptAudioFile handle
const var wf = Content.addAudioWaveform("Waveform1", 0, 0);
const var af = Engine.createAndRegisterAudioFile(0);
wf.referToData(af);
```

```json:testMetadata:referToData-audio-file
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "af.getNumSamples()", "value": 0},
    {"type": "REPL", "expression": "wf.getRangeEnd()", "value": 0}
  ]
}
```

---

## registerAtParent

**Signature:** `ScriptObject registerAtParent(Integer index)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Minimal Example:** `var af = {obj}.registerAtParent(0);`

**Description:**
Registers this component's internal audio data with the parent script processor at the given slot index, making it accessible from scriptnode or external code. Returns a `ScriptAudioFile` handle that can be used to programmatically load, query, and manipulate the audio data. The returned handle references the same audio buffer as this waveform component.

If the parent processor is not a `ProcessorWithDynamicExternalData`, returns an undefined value.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| index | Integer | no | The slot index to register at in the parent processor | >= 0 |

**Cross References:**
- `$API.ScriptAudioWaveform.referToData$`

**Example:**
```javascript:registerAtParent-usage
// Title: Register waveform data at parent and get audio file handle
const var wf = Content.addAudioWaveform("Waveform1", 0, 0);
const var af = wf.registerAtParent(0);
```

```json:testMetadata:registerAtParent-usage
{
  "testable": true,
  "verifyScript": {"type": "REPL", "expression": "af.getNumSamples()", "value": 0}
}
```

---

## setDefaultFolder

**Signature:** `undefined setDefaultFolder(ScriptObject newDefaultFolder)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setDefaultFolder(FileSystem.getFolder(FileSystem.Documents));`

**Description:**
Sets the default root directory for the file browser that opens when the user clicks on the waveform to load an audio file. The argument must be a `File` object obtained from `FileSystem.getFolder()` or similar File API calls. Passing a string path instead of a File object causes a script error.

This only affects AudioFile mode. In Sampler mode (when connected to a ModulatorSampler), the file browser is not used.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newDefaultFolder | ScriptObject | no | A File object representing the target directory | Must be a ScriptFile object |

**Pitfalls:**
- Passing a string path instead of a File object causes a script error: "newDefaultFolder must be a File object". Use `FileSystem.getFolder()` or equivalent to obtain a File object.
- If no audio file is loaded (the cached audio file is null), the call silently does nothing.

**Cross References:**
- `$API.ScriptAudioWaveform.referToData$`
- `$API.FileSystem.getFolder$`

---

## setPlaybackPosition

**Signature:** `undefined setPlaybackPosition(Double normalisedPosition)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setPlaybackPosition(0.5);`

**Description:**
Moves the playback cursor displayed on the waveform to the given normalized position. The position is a value from 0.0 to 1.0 representing the fraction of the current sample range. The position is converted to a sample index relative to the current range length and sent as an asynchronous display change message through the ComplexDataUIUpdater.

This is a display-only method -- it does not affect actual audio playback. It is used to visually indicate the current playback position by driving the waveform's playback indicator from script (e.g. from a timer callback tracking a player's position).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| normalisedPosition | Double | no | The playback position as a fraction of the current range | 0.0 to 1.0 |

**Pitfalls:**
- If no audio data is loaded (the cached audio file is null), the call silently does nothing.

**Cross References:**
- `$API.ScriptAudioWaveform.getRangeStart$`
- `$API.ScriptAudioWaveform.getRangeEnd$`

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
- `$API.ScriptAudioWaveform.changed$`

**Example:**
```javascript:custom-control-callback
// Title: Route a control callback to a per-component handler
const var btn = Content.addButton("Btn1", 0, 0);
btn.set("saveInPreset", false);

var callbackLog = [];

inline function onMyButton(component, value)
{
    callbackLog.push(component.getId() + ": " + value);
};

btn.setControlCallback(onMyButton);
```

```json:testMetadata:custom-control-callback
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "btn.setValue(1) || btn.changed()", "value": false},
    {"type": "REPL", "expression": "callbackLog[0]", "value": "Btn1: 1"}
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
- `$API.ScriptAudioWaveform.setConsumedKeyPresses$`

**Example:**
```javascript:key-press-handler
// Title: Log key presses on a focused panel
const var panel = Content.addPanel("Panel1", 0, 0);
panel.setConsumedKeyPresses("all");
panel.setKeyPressCallback(inline function(event)
{
    if (!event.isFocusChange)
        Console.print("Key: " + event.description);
});

// --- test-only ---
Console.testCallback(panel, "setKeyPressCallback", {
    "isFocusChange": false,
    "character": "A",
    "specialKey": false,
    "keyCode": 65,
    "description": "A"
});
// --- end test-only ---
```

```json:testMetadata:key-press-handler
{
  "testable": true,
  "verifyScript": {"type": "log-output", "values": ["Key: A"]}
}
```

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
- `$API.ScriptAudioWaveform.setKeyPressCallback$`

**Example:**
```javascript:consumed-key-formats
// Title: Three ways to specify consumed key presses
const var panel = Content.addPanel("Panel1", 0, 0);

// Consume all keys exclusively
panel.setConsumedKeyPresses("all");

// Consume specific keys by description string
panel.setConsumedKeyPresses(["ctrl + S", "F5", "escape"]);

// Consume specific keys by JSON object
panel.setConsumedKeyPresses([
    { "keyCode": 65, "ctrl": true },
    { "keyCode": 83, "ctrl": true }
]);

// Verify by registering a callback (would error if keys weren't consumed)
var keyLog = [];
panel.setKeyPressCallback(inline function(event)
{
    if (!event.isFocusChange)
        keyLog.push(event.keyCode);
});

// --- test-only ---
Console.testCallback(panel, "setKeyPressCallback", {
    "isFocusChange": false,
    "character": "A",
    "specialKey": false,
    "keyCode": 65,
    "description": "A"
});
// --- end test-only ---
```

```json:testMetadata:consumed-key-formats
{
  "testable": true,
  "verifyScript": {"type": "REPL", "expression": "keyLog[0]", "value": 65}
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
- `$API.ScriptAudioWaveform.setStyleSheetClass$`
- `$API.ScriptAudioWaveform.setStyleSheetPseudoState$`
- `$API.ScriptAudioWaveform.setLocalLookAndFeel$`

**Example:**
```javascript:css-variable-types
// Title: Set CSS variables with different unit types
const var knob = Content.addKnob("Knob1", 0, 0);
const var laf = Content.createLocalLookAndFeel();
knob.setLocalLookAndFeel(laf);

// Set a colour variable
knob.setStyleSheetProperty("track-color", 0xFFFF0000, "color");

// Set a size variable
knob.setStyleSheetProperty("track-width", 4, "px");

// Set a percentage variable
knob.setStyleSheetProperty("progress", 0.75, "%");
```

```json:testMetadata:css-variable-types
{
  "testable": true,
  "verifyScript": {"type": "REPL", "expression": "knob.getWidth()", "value": 128}
}
```

---

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
- `$API.ScriptAudioWaveform.setStyleSheetProperty$`
- `$API.ScriptAudioWaveform.setStyleSheetPseudoState$`
- `$API.ScriptAudioWaveform.setLocalLookAndFeel$`

**Example:**
```javascript:css-class-selectors
// Title: Assign CSS class selectors to a component
const var knob = Content.addKnob("Knob1", 0, 0);
const var laf = Content.createLocalLookAndFeel();
knob.setLocalLookAndFeel(laf);

// Add custom classes (component type class is auto-prepended)
knob.setStyleSheetClass(".large .highlighted");
// Result: ".scriptslider .large .highlighted"
```

```json:testMetadata:css-class-selectors
{
  "testable": true,
  "verifyScript": {"type": "REPL", "expression": "knob.getWidth()", "value": 128}
}
```

---

