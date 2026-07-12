# Content -- Method Documentation

## addVisualGuide

**Signature:** `undefined addVisualGuide(var guideData, var colour)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Iterates screenshotListeners and may trigger repaint operations.
**Minimal Example:** `Content.addVisualGuide([100, 0], Colours.red);`

**Description:**
Adds a visual overlay guide (line or rectangle) for layout debugging. Pass a 4-element array `[x, y, w, h]` for a rectangle, a 2-element array `[0, y]` for a horizontal line at y, or `[x, 0]` for a vertical line at x. Passing any non-array value (e.g., `0`) clears all existing guides. The colour parameter sets the guide's display colour.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| guideData | Array | no | Array defining the guide shape, or non-array to clear all guides | `[x,y,w,h]` for rect, `[0,y]` for h-line, `[x,0]` for v-line |
| colour | Colour | no | Display colour for the guide | 0xAARRGGBB or Colours constant |

**Pitfalls:**
- [BUG] A 2-element array with both values non-zero (e.g., `[50, 100]`) does not create any guide -- neither the horizontal nor vertical code path matches, and the VisualGuide is added with an uninitialized type and default area.

**Cross References:**
- `$API.Content.createScreenshot$`

**Example:**
```javascript:visual-guide-types
// Title: Adding different types of visual guides
// Add a horizontal line at y=100
Content.addVisualGuide([0, 100], Colours.red);

// Add a vertical line at x=200
Content.addVisualGuide([200, 0], Colours.blue);

// Add a rectangle guide
Content.addVisualGuide([10, 10, 300, 200], 0x4400FF00);

// Clear all guides
Content.addVisualGuide(0, 0);
```
```json:testMetadata:visual-guide-types
{
  "testable": false,
  "skipReason": "Visual guides are display-only overlays with no queryable state"
}
```

## callAfterDelay

**Signature:** `undefined callAfterDelay(int milliSeconds, Function function, var thisObject)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Creates a WeakCallbackHolder (heap allocation) and schedules a JUCE Timer callback on the message thread.
**Minimal Example:** `Content.callAfterDelay(500, onDelayedAction);`

**Description:**
Schedules a function to execute after the specified delay in milliseconds. The callback runs on the message thread via JUCE's Timer system, which is not sample-accurate -- do not use for DSP timing. An optional third argument sets the `this` context for the callback. The callback takes no arguments.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| milliSeconds | Integer | no | Delay before execution in milliseconds | > 0 |
| function | Function | no | Callback to execute after the delay | Takes 0 arguments |
| thisObject | ScriptObject | no | Optional `this` context for the callback | -- |

**Callback Signature:** function()

**Pitfalls:**
- The callback fires on the message thread, not the audio thread. It is not sample-accurate and should not be used for musical timing.
- The third parameter (thisObject) is optional in the wrapper (defaults to empty var if only 2 args are passed).
- [BUG] The wrapper returns `isMouseDown()` instead of undefined due to a copy-paste error. The method is void but script callers receive a spurious integer (0, 1, or 2).

**Cross References:**
- `$API.Engine.createTimerObject$`

**Example:**
```javascript:delayed-action
// Title: Execute a function after a delay
Content.makeFrontInterface(600, 300);

inline function onDelayedAction()
{
    Console.print("Executed after delay");
};

Content.callAfterDelay(200, onDelayedAction);
```
```json:testMetadata:delayed-action
{
  "testable": false,
  "skipReason": "Async timer callback fires after compile response is returned; log-output verification cannot capture deferred Console.print output"
}
```

## createLocalLookAndFeel

**Signature:** `ScriptLookAndFeel createLocalLookAndFeel()`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Heap-allocates a ScriptedLookAndFeel object and registers debug info listeners in backend builds.
**Minimal Example:** `var laf = Content.createLocalLookAndFeel();`

**Description:**
Creates a local Look and Feel object that can be used to customize the appearance of individual UI components. The created LAF is "local" (not global) -- it only applies to components that have it explicitly assigned via `component.setLocalLookAndFeel(laf)`. Register drawing functions on the returned object with `laf.registerFunction()`.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.ScriptLookAndFeel.registerFunction$`
- `$API.ScriptComponent.setLocalLookAndFeel$`
- `$API.Engine.createGlobalScriptLookAndFeel$`
- `$API.Content.createMarkdownRenderer$`

**Example:**
```javascript:local-laf-button
// Title: Creating a local LAF for a button
Content.makeFrontInterface(600, 300);

const var btn = Content.addButton("MyButton", 10, 10);

const var laf = Content.createLocalLookAndFeel();

laf.registerFunction("drawToggleButton", function(g, obj)
{
    g.fillAll(obj.value ? Colours.green : Colours.red);
    g.setColour(Colours.white);
    g.drawAlignedText(obj.text, [0, 0, obj.area[2], obj.area[3]], "centred");
});

btn.setLocalLookAndFeel(laf);
```
```json:testMetadata:local-laf-button
{
  "testable": false,
  "skipReason": "LAF rendering requires visual component display, cannot be verified via REPL"
}
```

## createMarkdownRenderer

**Signature:** `MarkdownRenderer createMarkdownRenderer()`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Heap-allocates a MarkdownObject.
**Minimal Example:** `var md = Content.createMarkdownRenderer();`

**Description:**
Creates a MarkdownRenderer object for rendering markdown text in ScriptPanel paint routines. After creation, call `setTextBounds()` to define the rendering area, then `setText()` to set the content, and pass the renderer to `Graphics.drawMarkdownText()` inside a paint routine.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.MarkdownRenderer.setText$`
- `$API.MarkdownRenderer.setTextBounds$`
- `$API.Graphics.drawMarkdownText$`
- `$API.Content.createLocalLookAndFeel$`

## createPath

**Signature:** `Path createPath()`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Heap-allocates a PathObject.
**Minimal Example:** `var p = Content.createPath();`

**Description:**
Creates a new Path object for vector drawing. Use Path methods like `startNewSubPath()`, `lineTo()`, `quadraticTo()`, `cubicTo()` to define the shape, then use it with `Graphics.fillPath()` or `Graphics.drawPath()` inside paint routines. Paths can also be loaded from base64 data via `Path.loadFromData()`.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.Path.startNewSubPath$`
- `$API.Path.lineTo$`
- `$API.Graphics.fillPath$`
- `$API.Graphics.drawPath$`
- `$API.Content.createSVG$`
- `$API.Content.createShader$`

## createScreenshot

**Signature:** `undefined createScreenshot(var area, ScriptObject directory, String name)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Blocking operation -- waits for all screenshot listeners (shaders, etc.) to be ready, involves file I/O.
**Minimal Example:** `Content.createScreenshot(myPanel, myDir, "screenshot");`

**Description:**
Creates a PNG screenshot of a specified area and saves it to a directory. The `area` parameter can be either a ScriptComponent reference (captures that component's global bounds) or a `[x, y, w, h]` array for arbitrary coordinates. The `directory` must be a ScriptFile object pointing to a directory (created if it does not exist). The `name` parameter is the file name (`.png` extension added automatically). This method blocks until all screenshot listeners (e.g., shaders) are ready.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| area | Object | no | Component reference or [x,y,w,h] array defining the capture area | -- |
| directory | ScriptObject | no | ScriptFile pointing to a target directory | Must be a directory |
| name | String | no | Output file name (without extension) | .png added automatically |

**Pitfalls:**
- Does nothing if no screenshot listeners are registered (the `screenshotListeners` array is empty). This typically means no shaders or visual elements that support screenshots are active.

**Cross References:**
- `$API.Content.addVisualGuide$`
- `$API.FileSystem.getFolder$`

## createShader

**Signature:** `ScriptShader createShader(String fileName)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Heap-allocates a ScriptShader object and registers it as a screenshot listener.
**Minimal Example:** `var shader = Content.createShader("myShader");`

**Description:**
Creates a ScriptShader object for GLSL-based rendering in ScriptPanels. If `fileName` is not empty, the fragment shader file is loaded from the project's shader folder. Pass an empty string to create a shader without loading a file immediately. The shader is automatically registered as a screenshot listener.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| fileName | String | no | Fragment shader filename (from the project's shader folder) | Empty string to skip loading |

**Cross References:**
- `$API.ScriptShader.setFragmentShader$`
- `$API.ScriptPanel.setShader$`
- `$API.Content.createPath$`
- `$API.Content.createSVG$`

## createSVG

**Signature:** `ScriptObject createSVG(String base64Data)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Heap-allocates an SVGObject, decompresses base64/zstd data, and parses XML asynchronously.
**Minimal Example:** `var svg = Content.createSVG(svgData);`

**Description:**
Creates an SVG object from base64-encoded, zstd-compressed SVG data. The encoded string should contain the compressed SVG XML. The SVG is parsed asynchronously on the message thread. Use the returned object with `Graphics.drawSVG()` in paint routines. The SVG data is typically generated by the HISE SVG tools.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| base64Data | String | no | Base64-encoded zstd-compressed SVG XML data | Must be valid base64 |

**Pitfalls:**
- The SVG parsing happens asynchronously (SafeAsyncCall). The SVG object may not be immediately valid after creation -- the drawable is set on the message thread after XML parsing completes.

**Cross References:**
- `$API.Graphics.drawSVG$`
- `$API.Content.createPath$`
- `$API.Content.createShader$`

## getAllComponents

**Signature:** `Array getAllComponents(String regex)`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Iterates all components and performs regex matching (String operations).
**Minimal Example:** `var comps = Content.getAllComponents("Knob.*");`

**Description:**
Returns an array of all component references whose names match the given regex pattern. Pass `".*"` to get all components (uses an optimized path that skips regex matching). The regex uses wildcard matching via `RegexFunctions::matchesWildcard()`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| regex | String | no | Regex pattern to match component names | ".*" for all |

**Cross References:**
- `$API.Content.getComponent$`
- `$API.Content.componentExists$`

## addDynamicContainer

**Signature:** `ScriptDynamicContainer addDynamicContainer(String containerId, int x, int y)`
**Return Type:** `ScriptDynamicContainer`
**Call Scope:** init
**Minimal Example:** `var dc = Content.addDynamicContainer("Container1", 0, 0);`

**Description:**
Creates a ScriptDynamicContainer component (a layout container that dynamically manages child components) and adds it to the interface. Accepts either 1 argument (name only, position defaults to 0,0) or 3 arguments (name, x, y). If a component with the same name already exists, the existing component is returned and its position is optionally updated. This idempotent behavior makes the call safe across recompiles. Can only be called during onInit -- calling after initialization throws a script error.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| containerId | String | no | Unique name for the component (also used as its ID) | No whitespace |
| x | Integer | no | Horizontal position in pixels | >= 0 (clamped via jmax) |
| y | Integer | no | Vertical position in pixels | >= 0 (clamped via jmax) |

**Pitfalls:**
- Calling this method after onInit throws "Tried to add a component after onInit()". All component creation must happen during onInit.

**Cross References:**
- `$API.Content.getComponent$`
- `$API.Content.componentExists$`

## addButton

**Signature:** `ScriptButton addButton(String buttonName, int x, int y)`
**Return Type:** `ScriptButton`
**Call Scope:** init
**Minimal Example:** `var btn = Content.addButton("MyButton", 10, 10);`

**Description:**
Creates a ScriptButton (toggle button) component and adds it to the interface. Accepts either 1 argument (name only, position defaults to 0,0) or 3 arguments (name, x, y). If a component with the same name already exists, the existing component is returned and its position is optionally updated. This idempotent behavior makes the call safe across recompiles. Can only be called during onInit -- calling after initialization throws a script error.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| buttonName | String | no | Unique name for the component (also used as its ID) | No whitespace |
| x | Integer | no | Horizontal position in pixels | >= 0 (clamped via jmax) |
| y | Integer | no | Vertical position in pixels | >= 0 (clamped via jmax) |

**Pitfalls:**
- Calling this method after onInit throws "Tried to add a component after onInit()". All component creation must happen during onInit.

**Cross References:**
- `$API.Content.getComponent$`
- `$API.Content.componentExists$`

**Example:**
```javascript:add-button-basic
// Title: Creating buttons with idempotent naming
const var btn1 = Content.addButton("BypassBtn", 10, 10);
const var btn2 = Content.addButton("BypassBtn", 10, 10);
Console.assertTrue(btn1 == btn2);
```
```json:testMetadata:add-button-basic
{
  "testable": true,
  "verifyScript": {"type": "REPL", "expression": "btn1 == btn2", "value": true}
}
```

## addComboBox

**Signature:** `ScriptComboBox addComboBox(String boxName, int x, int y)`
**Return Type:** `ScriptComboBox`
**Call Scope:** init
**Minimal Example:** `var cb = Content.addComboBox("ModeSelector", 10, 10);`

**Description:**
Creates a ScriptComboBox (dropdown selector) component and adds it to the interface. Accepts either 1 argument (name only, position defaults to 0,0) or 3 arguments (name, x, y). If a component with the same name already exists, the existing component is returned and its position is optionally updated. This idempotent behavior makes the call safe across recompiles. Can only be called during onInit -- calling after initialization throws a script error.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| boxName | String | no | Unique name for the component (also used as its ID) | No whitespace |
| x | Integer | no | Horizontal position in pixels | >= 0 (clamped via jmax) |
| y | Integer | no | Vertical position in pixels | >= 0 (clamped via jmax) |

**Pitfalls:**
- Calling this method after onInit throws "Tried to add a component after onInit()". All component creation must happen during onInit.

**Cross References:**
- `$API.Content.getComponent$`
- `$API.Content.componentExists$`

## addFloatingTile

**Signature:** `ScriptFloatingTile addFloatingTile(String floatingTileName, int x, int y)`
**Return Type:** `ScriptFloatingTile`
**Call Scope:** init
**Minimal Example:** `var ft = Content.addFloatingTile("PresetBrowser", 10, 10);`

**Description:**
Creates a ScriptFloatingTile component (an embedded floating tile for preset browsers, keyboards, and other built-in widgets) and adds it to the interface. Accepts either 1 argument (name only, position defaults to 0,0) or 3 arguments (name, x, y). If a component with the same name already exists, the existing component is returned and its position is optionally updated. This idempotent behavior makes the call safe across recompiles. Can only be called during onInit -- calling after initialization throws a script error.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| floatingTileName | String | no | Unique name for the component (also used as its ID) | No whitespace |
| x | Integer | no | Horizontal position in pixels | >= 0 (clamped via jmax) |
| y | Integer | no | Vertical position in pixels | >= 0 (clamped via jmax) |

**Pitfalls:**
- Calling this method after onInit throws "Tried to add a component after onInit()". All component creation must happen during onInit.

**Cross References:**
- `$API.Content.getComponent$`
- `$API.Content.componentExists$`

## addImage

**Signature:** `ScriptImage addImage(String imageName, int x, int y)`
**Return Type:** `ScriptImage`
**Call Scope:** init
**Minimal Example:** `var img = Content.addImage("Logo", 10, 10);`

**Description:**
Creates a ScriptImage component (a static image display) and adds it to the interface. Accepts either 1 argument (name only, position defaults to 0,0) or 3 arguments (name, x, y). If a component with the same name already exists, the existing component is returned and its position is optionally updated. This idempotent behavior makes the call safe across recompiles. Can only be called during onInit -- calling after initialization throws a script error.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| imageName | String | no | Unique name for the component (also used as its ID) | No whitespace |
| x | Integer | no | Horizontal position in pixels | >= 0 (clamped via jmax) |
| y | Integer | no | Vertical position in pixels | >= 0 (clamped via jmax) |

**Pitfalls:**
- Calling this method after onInit throws "Tried to add a component after onInit()". All component creation must happen during onInit.

**Cross References:**
- `$API.Content.getComponent$`
- `$API.Content.componentExists$`

## addKnob

**Signature:** `ScriptSlider addKnob(String knobName, int x, int y)`
**Return Type:** `ScriptSlider`
**Call Scope:** init
**Minimal Example:** `var knob = Content.addKnob("Volume", 10, 10);`

**Description:**
Creates a ScriptSlider component (rotary knob or linear slider, depending on the `mode` property) and adds it to the interface. Despite the method name, this creates a ScriptSlider object -- the name `addKnob` reflects the default rotary appearance. Accepts either 1 argument (name only, position defaults to 0,0) or 3 arguments (name, x, y). If a component with the same name already exists, the existing component is returned and its position is optionally updated. This idempotent behavior makes the call safe across recompiles. Can only be called during onInit -- calling after initialization throws a script error.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| knobName | String | no | Unique name for the component (also used as its ID) | No whitespace |
| x | Integer | no | Horizontal position in pixels | >= 0 (clamped via jmax) |
| y | Integer | no | Vertical position in pixels | >= 0 (clamped via jmax) |

**Pitfalls:**
- Calling this method after onInit throws "Tried to add a component after onInit()". All component creation must happen during onInit.

**Cross References:**
- `$API.Content.getComponent$`
- `$API.Content.componentExists$`

**Example:**
```javascript:add-knob-basic
// Title: Creating knobs and accessing the returned reference
Content.makeFrontInterface(600, 300);
const var vol = Content.addKnob("VolKn", 10, 10);
vol.setRange(0.0, 1.0, 0.01);
vol.set("defaultValue", 0.75);
```
```json:testMetadata:add-knob-basic
{
  "testable": true,
  "verifyScript": {"type": "REPL", "expression": "vol.getRange().max", "value": 1.0}
}
```

## addLabel

**Signature:** `ScriptLabel addLabel(String label, int x, int y)`
**Return Type:** `ScriptLabel`
**Call Scope:** init
**Minimal Example:** `var lbl = Content.addLabel("StatusText", 10, 10);`

**Description:**
Creates a ScriptLabel component (a text input/display label) and adds it to the interface. Accepts either 1 argument (name only, position defaults to 0,0) or 3 arguments (name, x, y). If a component with the same name already exists, the existing component is returned and its position is optionally updated. This idempotent behavior makes the call safe across recompiles. Can only be called during onInit -- calling after initialization throws a script error.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| label | String | no | Unique name for the component (also used as its ID) | No whitespace |
| x | Integer | no | Horizontal position in pixels | >= 0 (clamped via jmax) |
| y | Integer | no | Vertical position in pixels | >= 0 (clamped via jmax) |

**Pitfalls:**
- Calling this method after onInit throws "Tried to add a component after onInit()". All component creation must happen during onInit.

**Cross References:**
- `$API.Content.getComponent$`
- `$API.Content.componentExists$`

## addMultipageDialog

**Signature:** `ScriptMultipageDialog addMultipageDialog(String dialogId, int x, int y)`
**Return Type:** `ScriptMultipageDialog`
**Call Scope:** init
**Minimal Example:** `var dlg = Content.addMultipageDialog("InstallerDialog", 10, 10);`

**Description:**
Creates a ScriptMultipageDialog component (a multi-page dialog for installers, setup wizards, etc.) and adds it to the interface. Accepts either 1 argument (name only, position defaults to 0,0) or 3 arguments (name, x, y). If a component with the same name already exists, the existing component is returned and its position is optionally updated. This idempotent behavior makes the call safe across recompiles. Can only be called during onInit -- calling after initialization throws a script error.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| dialogId | String | no | Unique name for the component (also used as its ID) | No whitespace |
| x | Integer | no | Horizontal position in pixels | >= 0 (clamped via jmax) |
| y | Integer | no | Vertical position in pixels | >= 0 (clamped via jmax) |

**Pitfalls:**
- Calling this method after onInit throws "Tried to add a component after onInit()". All component creation must happen during onInit.

**Cross References:**
- `$API.Content.getComponent$`
- `$API.Content.componentExists$`

## addPanel

**Signature:** `ScriptPanel addPanel(String panelName, int x, int y)`
**Return Type:** `ScriptPanel`
**Call Scope:** init
**Minimal Example:** `var pnl = Content.addPanel("BackgroundPanel", 0, 0);`

**Description:**
Creates a ScriptPanel component (a drawable panel with paint routines and mouse callbacks) and adds it to the interface. ScriptPanel is the most versatile component type -- it supports custom paint routines, mouse/key event callbacks, animation timers, drag-and-drop, and child component parenting. Accepts either 1 argument (name only, position defaults to 0,0) or 3 arguments (name, x, y). If a component with the same name already exists, the existing component is returned and its position is optionally updated. This idempotent behavior makes the call safe across recompiles. Can only be called during onInit -- calling after initialization throws a script error.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| panelName | String | no | Unique name for the component (also used as its ID) | No whitespace |
| x | Integer | no | Horizontal position in pixels | >= 0 (clamped via jmax) |
| y | Integer | no | Vertical position in pixels | >= 0 (clamped via jmax) |

**Pitfalls:**
- Calling this method after onInit throws "Tried to add a component after onInit()". All component creation must happen during onInit.

**Cross References:**
- `$API.Content.getComponent$`
- `$API.Content.componentExists$`

## addSliderPack

**Signature:** `ScriptSliderPack addSliderPack(String sliderPackName, int x, int y)`
**Return Type:** `ScriptSliderPack`
**Call Scope:** init
**Minimal Example:** `var sp = Content.addSliderPack("EQBands", 10, 10);`

**Description:**
Creates a ScriptSliderPack component (an array of sliders for multi-value editing like EQ bands or step sequencers) and adds it to the interface. Accepts either 1 argument (name only, position defaults to 0,0) or 3 arguments (name, x, y). If a component with the same name already exists, the existing component is returned and its position is optionally updated. This idempotent behavior makes the call safe across recompiles. Can only be called during onInit -- calling after initialization throws a script error.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| sliderPackName | String | no | Unique name for the component (also used as its ID) | No whitespace |
| x | Integer | no | Horizontal position in pixels | >= 0 (clamped via jmax) |
| y | Integer | no | Vertical position in pixels | >= 0 (clamped via jmax) |

**Pitfalls:**
- Calling this method after onInit throws "Tried to add a component after onInit()". All component creation must happen during onInit.

**Cross References:**
- `$API.Content.getComponent$`
- `$API.Content.componentExists$`

## addTable

**Signature:** `ScriptTable addTable(String tableName, int x, int y)`
**Return Type:** `ScriptTable`
**Call Scope:** init
**Minimal Example:** `var tbl = Content.addTable("VelocityCurve", 10, 10);`

**Description:**
Creates a ScriptTable component (a curve editor for mapping tables) and adds it to the interface. Accepts either 1 argument (name only, position defaults to 0,0) or 3 arguments (name, x, y). If a component with the same name already exists, the existing component is returned and its position is optionally updated. This idempotent behavior makes the call safe across recompiles. Can only be called during onInit -- calling after initialization throws a script error.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| tableName | String | no | Unique name for the component (also used as its ID) | No whitespace |
| x | Integer | no | Horizontal position in pixels | >= 0 (clamped via jmax) |
| y | Integer | no | Vertical position in pixels | >= 0 (clamped via jmax) |

**Pitfalls:**
- Calling this method after onInit throws "Tried to add a component after onInit()". All component creation must happen during onInit.

**Cross References:**
- `$API.Content.getComponent$`
- `$API.Content.componentExists$`

## addViewport

**Signature:** `ScriptedViewport addViewport(String viewportName, int x, int y)`
**Return Type:** `ScriptedViewport`
**Call Scope:** init
**Minimal Example:** `var vp = Content.addViewport("ScrollArea", 10, 10);`

**Description:**
Creates a ScriptedViewport component (a scrollable viewport for child content) and adds it to the interface. Accepts either 1 argument (name only, position defaults to 0,0) or 3 arguments (name, x, y). If a component with the same name already exists, the existing component is returned and its position is optionally updated. This idempotent behavior makes the call safe across recompiles. Can only be called during onInit -- calling after initialization throws a script error.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| viewportName | String | no | Unique name for the component (also used as its ID) | No whitespace |
| x | Integer | no | Horizontal position in pixels | >= 0 (clamped via jmax) |
| y | Integer | no | Vertical position in pixels | >= 0 (clamped via jmax) |

**Pitfalls:**
- Calling this method after onInit throws "Tried to add a component after onInit()". All component creation must happen during onInit.

**Cross References:**
- `$API.Content.getComponent$`
- `$API.Content.componentExists$`

## addWebView

**Signature:** `ScriptWebView addWebView(String webviewName, int x, int y)`
**Return Type:** `ScriptWebView`
**Call Scope:** init
**Minimal Example:** `var wv = Content.addWebView("HelpView", 10, 10);`

**Description:**
Creates a ScriptWebView component (an embedded web browser view) and adds it to the interface. Accepts either 1 argument (name only, position defaults to 0,0) or 3 arguments (name, x, y). If a component with the same name already exists, the existing component is returned and its position is optionally updated. This idempotent behavior makes the call safe across recompiles. Can only be called during onInit -- calling after initialization throws a script error.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| webviewName | String | no | Unique name for the component (also used as its ID) | No whitespace |
| x | Integer | no | Horizontal position in pixels | >= 0 (clamped via jmax) |
| y | Integer | no | Vertical position in pixels | >= 0 (clamped via jmax) |

**Pitfalls:**
- Calling this method after onInit throws "Tried to add a component after onInit()". All component creation must happen during onInit.

**Cross References:**
- `$API.Content.getComponent$`
- `$API.Content.componentExists$`

## componentExists

**Signature:** `bool componentExists(String name)`
**Return Type:** `bool`
**Call Scope:** safe
**Minimal Example:** `var exists = Content.componentExists("MyKnob");`

**Description:**
Checks whether a component with the given name exists on the interface. Returns `true` if a component has been created with that name via any `addXXX()` method, `false` otherwise. This is safe to call at any time (not restricted to onInit). Useful for conditional logic when scripts need to handle interfaces with optional components.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| name | String | no | The component name to check | Case-sensitive |

**Cross References:**
- `$API.Content.getComponent$`
- `$API.Content.getAllComponents$`

**Example:**
```javascript:component-exists-check
// Title: Conditional component access
Content.makeFrontInterface(600, 300);
const var knob = Content.addKnob("CExKnob1", 10, 10);
const var found = Content.componentExists("CExKnob1");
const var missing = Content.componentExists("NonExistent");
```
```json:testMetadata:component-exists-check
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "found", "value": true},
    {"type": "REPL", "expression": "missing", "value": false}
  ]
}
```

## setPropertiesFromJSON

**Signature:** `void setPropertiesFromJSON(String name, var jsonData)`
**Return Type:** `void`
**Call Scope:** unsafe
**Minimal Example:** `Content.setPropertiesFromJSON("MyKnob", {"text": "Volume", "width": 128, "height": 48});`

**Description:**
Sets multiple properties on a component identified by name using a JSON object. Each key in the JSON object must match a valid component property name (e.g., `text`, `width`, `height`, `x`, `y`, `visible`, `enabled`). This is a Content-level method that takes the component name as the first argument -- unlike `ScriptComponent.setPropertiesFromJSON()` which is called on the component reference directly.

If the named component does not exist, a script error is thrown.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| name | String | no | The name of the target component | Must exist |
| jsonData | JSON | no | Object with property name/value pairs | Keys must be valid property names |

**Pitfalls:**
- If a property key in the JSON does not match any valid property of the target component, it is silently ignored.

**Cross References:**
- `$API.Content.getComponent$`
- `$API.Content.componentExists$`

**Example:**
```javascript:set-props-from-json
// Title: Bulk-setting component properties
Content.makeFrontInterface(600, 300);
Content.addKnob("SPKnob1", 10, 10);
Content.setPropertiesFromJSON("SPKnob1", {
  "text": "Volume",
  "width": 128,
  "height": 48,
  "min": 0.0,
  "max": 1.0
});
const var ref = Content.getComponent("SPKnob1");
```
```json:testMetadata:set-props-from-json
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "ref.get(\"width\")", "value": 128},
    {"type": "REPL", "expression": "ref.get(\"text\")", "value": "Volume"}
  ]
}
```

## storeAllControlsAsPreset

**Signature:** `void storeAllControlsAsPreset(String fileName, var automationData)`
**Return Type:** `void`
**Call Scope:** unsafe
**Minimal Example:** `Content.storeAllControlsAsPreset("myPreset.xml", undefined);`

**Description:**
Saves all component control values to an XML data file. This is the counterpart to `restoreAllControlsFromPreset()`. The `fileName` is resolved relative to the `UserPresets` folder. The second argument `automationData` is a ValueTree that, when provided, filters which controls are included based on plugin parameter automation data. Pass `undefined` to store all controls.

In exported plugins (USE_FRONTEND), the filename is matched against the embedded preset list -- the file path does not need to reference an actual disk file.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| fileName | String | no | The preset file name (relative to UserPresets) | Should end in .xml |
| automationData | var | no | Optional automation filter data (pass `undefined` for all controls) | ValueTree or undefined |

**Pitfalls:**
- The file is written relative to the UserPresets directory. Using absolute paths or paths outside this directory may not work as expected.

**Cross References:**
- `$API.Content.restoreAllControlsFromPreset$`

## getComponent

**Signature:** `ScriptComponent getComponent(var componentName)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Linear search through components array (String comparison per element).
**Minimal Example:** `var knob = Content.getComponent("MyKnob");`

**Description:**
Returns a reference to the component with the given name. Performs a linear search through all registered components. If no component with the specified name is found, logs an error and returns undefined. In the HISE IDE (backend), this method also supports the "throw at definition" mechanism for IDE navigation.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| componentName | String | no | The name of the component to find | Must match a created component |

**Pitfalls:**
- Unlike `Content.addKnob()` etc., this method does not create a component -- it only retrieves existing ones. If the component does not exist, an error is logged but execution continues (returns undefined).
- Calling `Content.getComponent()` inside callbacks, timer functions, or paint routines is a common performance mistake. Each call performs a linear search through all components. In a plugin with hundreds of components, this overhead is measurable. Always cache references as `const var` at init time.

**Cross References:**
- `$API.Content.getAllComponents$`

**Example:**
```javascript:caching-component-refs
// Title: Caching component references at init time
// --- setup ---
Content.addKnob("GainKnob", 10, 10);
Content.addKnob("MixKnob", 150, 10);
Content.addButton("BypassBtn", 300, 10);
for (i = 0; i < 4; i++)
{
    Content.addKnob("Volume" + (i + 1), 10, 60 + i * 50);
    Content.addKnob("Pan" + (i + 1), 150, 60 + i * 50);
}
// --- end setup ---
// Context: getComponent performs a linear search. The standard practice is to
// cache all references as const var during onInit and use those variables
// everywhere else. This is the single most common API call in HiseScript.

Content.makeFrontInterface(900, 600);

// Cache references once at init
const var gainKnob = Content.getComponent("GainKnob");
const var mixKnob = Content.getComponent("MixKnob");
const var bypassBtn = Content.getComponent("BypassBtn");

// Build arrays of related components using a loop
const var NUM_CHANNELS = 4;
const var channelVolumes = [];
const var channelPans = [];

for (i = 0; i < NUM_CHANNELS; i++)
{
    channelVolumes.push(Content.getComponent("Volume" + (i + 1)));
    channelPans.push(Content.getComponent("Pan" + (i + 1)));
}

Console.print(channelVolumes.length); // 4
```
```json:testMetadata:caching-component-refs
{
  "testable": true,
  "verifyScript": [
    {
      "type": "REPL",
      "expression": "gainKnob.get(\"id\")",
      "value": "GainKnob"
    },
    {
      "type": "REPL",
      "expression": "channelVolumes.length",
      "value": 4
    },
    {
      "type": "REPL",
      "expression": "channelVolumes[2].get(\"id\")",
      "value": "Volume3"
    }
  ]
}
```

## getComponentUnderDrag

**Signature:** `String getComponentUnderDrag()`
**Return Type:** `String`
**Call Scope:** safe
**Call Scope Note:** Iterates RebuildListeners (lock-free observer list) and returns a string.
**Minimal Example:** `var id = Content.getComponentUnderDrag();`

**Description:**
Returns the component ID of the component currently being dragged, by querying registered RebuildListeners via the `DragAction::Query` action. Returns an empty string if no drag operation is active. This is part of the Content drag operation system.

**Cross References:**
- `$API.Content.refreshDragImage$`
- `$API.Content.getComponentUnderMouse$`

## getComponentUnderMouse

**Signature:** `String getComponentUnderMouse()`
**Return Type:** `String`
**Call Scope:** safe
**Call Scope Note:** Queries JUCE Desktop for the component under the mouse cursor -- no allocations or locks.
**Minimal Example:** `var id = Content.getComponentUnderMouse();`

**Description:**
Returns the JUCE component ID of whatever JUCE Component is currently under the mouse cursor. Note: this returns the JUCE-level component ID, which may not directly correspond to a HISEScript component name.

**Pitfalls:**
- Returns the JUCE component ID, not the HISEScript component name. These may differ depending on the component wrapping hierarchy.

**Cross References:**
- `$API.Content.getComponentUnderDrag$`
- `$API.Content.isMouseDown$`

## getCurrentTooltip

**Signature:** `String getCurrentTooltip()`
**Return Type:** `String`
**Call Scope:** safe
**Call Scope Note:** Queries JUCE Desktop for mouse source and TooltipClient interface -- no allocations.
**Minimal Example:** `var tip = Content.getCurrentTooltip();`

**Description:**
Returns the tooltip text of the component currently under the mouse cursor. Uses JUCE's `TooltipClient` interface to query the tooltip. Returns an empty string if no tooltip-capable component is under the mouse or if touch input is active.

**Cross References:**
- `$API.Content.setContentTooltip$`
- `$API.ScriptComponent.set$` (tooltip property)`

**Example:**
```javascript:example
// Title: Example
const var t = Engine.createTimerObject();
const var Label1 = Content.getComponent("Label1");
reg isPending = false;

t.setTimerCallback(function()
{
	var tooltip = Content.getCurrentTooltip();
	
	if(tooltip == "")
	{
		// Now the mouse is over a component without a tooltip
	
		if(Label1.get("text") != "" && !isPending)
		{
			// The tooltip label was not empty so we set the isPending flag
			// and reset the internal counter of the timer object
			isPending = true;
			this.resetCounter(); // [1]
		}
		else if (this.getMilliSecondsSinceCounterReset() > 1000)
		{
			// Now a second has passed since [1] without a new tooltip being
			// set, so we clear the label and reset the isPending flag
			isPending = false;
			Label1.set("text", "");
		}
	}
	else
	{
		// We update the label with the new tooltip and
		// clear the isPending flag
		isPending = false;
		Label1.set("text", tooltip);
	}
});

// We don't need it to be super fast, so 100ms should be fine
t.startTimer(100);
```

## getInterfaceSize

**Signature:** `Array getInterfaceSize()`
**Return Type:** `Array`
**Call Scope:** safe
**Call Scope Note:** Returns two integer values from member variables -- no allocations beyond the array construction.
**Minimal Example:** `var size = Content.getInterfaceSize();`

**Description:**
Returns the current interface dimensions as a `[width, height]` array.

**Cross References:**
- `$API.Content.setWidth$`
- `$API.Content.setHeight$`
- `$API.Content.makeFrontInterface$`

**Example:**
```javascript:get-interface-size
// Title: Querying the interface dimensions
Content.makeFrontInterface(800, 500);

const var size = Content.getInterfaceSize();
Console.print(size[0]);
Console.print(size[1]);
```
```json:testMetadata:get-interface-size
{
  "testable": true,
  "verifyScript": [
    {
      "type": "REPL",
      "expression": "size[0]",
      "value": 800
    },
    {
      "type": "REPL",
      "expression": "size[1]",
      "value": 500
    }
  ]
}
```

## getScreenBounds

**Signature:** `Array getScreenBounds(Integer getTotalArea)`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Acquires MessageManagerLock -- blocks if not on the message thread.
**Minimal Example:** `var bounds = Content.getScreenBounds(true);`

**Description:**
Returns the screen bounds as an `[x, y, width, height]` array. When `getTotalArea` is true, returns the total display area including taskbar/dock. When false, returns only the user-available area (excluding taskbar). Uses JUCE's `Desktop::getDisplays()` for the main display.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| getTotalArea | Integer | no | True for total screen area, false for user area only | -- |

**Pitfalls:**
- Acquires a `MessageManagerLock` internally. Calling this from the audio thread will block until the message thread is free, which may cause audio dropouts.

**Cross References:**
- `$API.Content.getInterfaceSize$`

**Example:**
```javascript:drag-to-zoom-handler
// Title: Implementing a drag-to-zoom handler with screen bounds clamping
// Context: Commercial plugins implement zoom by letting the user drag a corner
// panel. getScreenBounds provides the display height needed to calculate the
// maximum safe zoom level that won't exceed the screen.

Content.makeFrontInterface(900, 600);

const var zoomPanel = Content.addPanel("ZoomPanel", 850, 550);
zoomPanel.set("width", 50);
zoomPanel.set("height", 50);

namespace ZoomHandler
{
    const var MIN_ZOOM = 0.75;
    const var MAX_ZOOM = 2.0;
    const var ZOOM_STEP = 0.25;
    const var INTERFACE_HEIGHT = 600;

    zoomPanel.setMouseCallback(function(event)
    {
        if (event.clicked)
            this.data.zoomStart = Settings.getZoomLevel();

        if (event.drag)
        {
            var currentZoom = Settings.getZoomLevel();

            // Compute drag delta as a fraction of interface height
            local dragPixel = (event.dragY * currentZoom) / INTERFACE_HEIGHT;

            // Clamp to screen height using getScreenBounds
            // false = user area only (excludes taskbar)
            local maxZoom = Content.getScreenBounds(false)[3] / INTERFACE_HEIGHT;

            local newZoom = this.data.zoomStart + dragPixel;

            // Snap to step increments
            newZoom += (ZOOM_STEP / 2);
            newZoom = Math.min(newZoom, maxZoom);
            newZoom -= Math.fmod(newZoom, ZOOM_STEP);
            newZoom = Math.range(newZoom, MIN_ZOOM, MAX_ZOOM);

            if (currentZoom != newZoom)
                Settings.setZoomLevel(newZoom);
        }
    });
}
```
```json:testMetadata:drag-to-zoom-handler
{
  "testable": false,
  "skipReason": "Mouse drag callback requires user interaction and Settings.setZoomLevel has visual side-effects"
}
```

## isCtrlDown

**Signature:** `Integer isCtrlDown()`
**Return Type:** `Integer`
**Call Scope:** safe
**Call Scope Note:** Reads static JUCE ModifierKeys state -- no allocations or locks.
**Minimal Example:** `var ctrl = Content.isCtrlDown();`

**Description:**
Returns true (1) if either the Ctrl key (Windows/Linux) or Command key (macOS) is currently pressed. This provides cross-platform modifier detection -- on macOS, it checks both Command and Ctrl.

**Cross References:**
- `$API.Content.isMouseDown$`

## isMouseDown

**Signature:** `Integer isMouseDown()`
**Return Type:** `Integer`
**Call Scope:** safe
**Call Scope Note:** Reads JUCE Desktop mouse source modifiers -- no allocations or locks.
**Minimal Example:** `var mouseState = Content.isMouseDown();`

**Description:**
Returns the current mouse button state: 0 if no button is pressed, 1 if the left button is down, 2 if the right button is down.

**Cross References:**
- `$API.Content.isCtrlDown$`
- `$API.Content.getComponentUnderMouse$`

## makeFrontInterface

**Signature:** `undefined makeFrontInterface(int width, int height)`
**Return Type:** `undefined`
**Call Scope:** init
**Call Scope Note:** Calls addToFront(true) which registers this script as the front interface -- must be called during onInit.
**Minimal Example:** `Content.makeFrontInterface(900, 600);`

**Description:**
Sets the interface dimensions and registers this script processor as the front interface. This is typically the first call in onInit for the main interface script. It sets both width and height, broadcasts the size change, and calls `addToFront(true)` on the parent JavascriptMidiProcessor.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| width | Integer | no | Interface width in pixels | > 0 |
| height | Integer | no | Interface height in pixels | > 0 |

**Cross References:**
- `$API.Content.setWidth$`
- `$API.Content.setHeight$`
- `$API.Content.makeFullScreenInterface$`
- `$API.Content.getInterfaceSize$`

**Example:**
```javascript:init-scaffold
// Title: Standard interface initialization scaffold
// Context: This is always the very first line of the main interface script.
// It sets the interface size and registers this script processor as the
// front interface. Everything else follows after this call.

Content.makeFrontInterface(900, 600);

// Optional: enable HiDPI for custom-drawn panels
Content.setUseHighResolutionForPanels(true);

// Create the UI components
const var mainPanel = Content.addPanel("MainPanel", 0, 0);
mainPanel.set("width", 900);
mainPanel.set("height", 600);

const var gainKnob = Content.addKnob("GainKnob", 10, 10);
const var bypassBtn = Content.addButton("BypassBtn", 150, 10);

const var size = Content.getInterfaceSize();
Console.print(size[0]); // 900
Console.print(size[1]); // 600
```
```json:testMetadata:init-scaffold
{
  "testable": true,
  "verifyScript": [
    {
      "type": "REPL",
      "expression": "size[0]",
      "value": 900
    },
    {
      "type": "REPL",
      "expression": "size[1]",
      "value": 600
    }
  ]
}
```

## makeFullScreenInterface

**Signature:** `undefined makeFullScreenInterface()`
**Return Type:** `undefined`
**Call Scope:** init
**Call Scope Note:** Calls addToFront(true) -- must be called during onInit.
**Minimal Example:** `Content.makeFullScreenInterface();`

**Description:**
Sets the interface dimensions to the device simulator's display resolution and registers this script as the front interface. Uses `HiseDeviceSimulator::getDisplayResolution()` to determine the size. This is intended for full-screen interfaces.

**Cross References:**
- `$API.Content.makeFrontInterface$`

## refreshDragImage

**Signature:** `Integer refreshDragImage()`
**Return Type:** `Integer`
**Call Scope:** safe
**Call Scope Note:** Iterates RebuildListeners (lock-free) and triggers repaint via DragAction::Repaint.
**Minimal Example:** `var refreshed = Content.refreshDragImage();`

**Description:**
Triggers a repaint of the current drag image by notifying RebuildListeners via the `DragAction::Repaint` action. Returns true (1) if a listener handled the repaint, false (0) otherwise.

**Cross References:**
- `$API.Content.getComponentUnderDrag$`

## restoreAllControlsFromPreset

**Signature:** `undefined restoreAllControlsFromPreset(String fileName)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Involves file I/O (reading XML from disk in backend) or ValueTree lookup (in frontend).
**Minimal Example:** `Content.restoreAllControlsFromPreset("MyPreset.preset");`

**Description:**
Restores all component values from a saved preset file. Accepts either an absolute path or a path relative to the UserPresets folder. In frontend (exported plugin) builds, reads from the embedded ValueTree hierarchy. In backend (HISE IDE) builds, reads from an XML file on disk. Components with `saveInPreset` set to false are skipped.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| fileName | String | no | Preset file path (absolute or relative to UserPresets) | Must be a valid XML preset file |

**Pitfalls:**
- In frontend builds, the file name is matched against the embedded `FileName` property in the ValueTree. A mismatch results in a "Preset ID not found" error.
- If the file exists but the processor ID does not match any child in the preset XML, a "Preset ID not found" error is thrown.

**Cross References:**
- `$API.UserPresetHandler.loadUserPreset$`

## setContentTooltip

**Signature:** `undefined setContentTooltip(String tooltipText)`
**Return Type:** `undefined`
**Call Scope:** safe
**Call Scope Note:** Simple string assignment to a member variable.
**Minimal Example:** `Content.setContentTooltip("My Plugin v1.0");`

**Description:**
Sets the tooltip text for the entire content area. This tooltip appears when hovering over the background of the interface (where no component is present).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| tooltipText | String | no | Tooltip text for the content background | -- |

**Cross References:**
- `$API.Content.getCurrentTooltip$`

## setHeight

**Signature:** `undefined setHeight(int newHeight)`
**Return Type:** `undefined`
**Call Scope:** safe
**Call Scope Note:** Simple integer comparison and assignment. The async broadcast is dispatched but does not block.
**Minimal Example:** `Content.setHeight(600);`

**Description:**
Sets the interface height in pixels. Only broadcasts a size change if the new value differs from the current height AND the width is non-zero (preventing spurious resize events during initial setup). Use `makeFrontInterface()` to set both dimensions at once.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newHeight | Integer | no | Interface height in pixels | > 0 |

**Pitfalls:**
- If `setHeight` is called before `setWidth` (width is still 0), no size broadcast is sent. The resize only takes effect after width is also set.

**Cross References:**
- `$API.Content.setWidth$`
- `$API.Content.makeFrontInterface$`
- `$API.Content.getInterfaceSize$`

## setKeyPressCallback

**Signature:** `undefined setKeyPressCallback(var keyPress, var callback)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Parses key press description (String operations, KeyPress construction) and modifies the registeredKeyPresses array.
**Minimal Example:** `Content.setKeyPressCallback("ctrl + s", onSaveKeyPress);`

**Description:**
Registers a keyboard shortcut callback at the interface level. The `keyPress` parameter can be a string description (e.g., `"ctrl + a"`, `"shift + F5"`) parsed by `KeyPress::createFromDescription()`, or a JSON object with `keyCode`, `character`, `shift`, `cmd`/`ctrl`, `alt` properties. If `callback` is a valid function, it is registered (replacing any existing callback for that key). If `callback` is not a function, the registration for that key press is removed.

The callback receives a single argument: a JSON object with properties describing the key event.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| keyPress | Object | no | Key press description (string or JSON object) | Must parse to a valid KeyPress |
| callback | Function | no | Callback for the key event, or non-function to unregister | -- |

**Pitfalls:**
- Keyboard shortcut callbacks use plain `function()` syntax (not `inline function`) because they fire on the message thread, not the audio thread. This is correct and expected.
- The callback receives a JSON event object with properties like `description`, `keyCode`, `shift`, `cmd`, `alt`, but for simple shortcuts the callback typically ignores the argument and performs a direct action.

**Cross References:**
- `$API.ScriptComponent.setKeyPressCallback$`

**Example:**
```javascript:keyboard-shortcut-navigation
// Title: Registering keyboard shortcuts for navigation
// Context: Plugins register keyboard shortcuts in a dedicated namespace,
// typically guarded by a user preference. Each shortcut triggers an action
// through the plugin's broadcaster system or by toggling a button.

Content.makeFrontInterface(900, 600);

const var settingsPanel = Content.addPanel("SettingsPanel", 0, 0);
settingsPanel.set("visible", false);
const var mixerPanel = Content.addPanel("MixerPanel", 0, 0);
mixerPanel.set("visible", false);

// Toggle between two main views
Content.setKeyPressCallback("s", function()
{
    settingsPanel.set("visible", !settingsPanel.get("visible"));
});

// Toggle mixer panel visibility
Content.setKeyPressCallback("m", function()
{
    mixerPanel.set("visible", !mixerPanel.get("visible"));
});

// Use modifier keys for less common actions
Content.setKeyPressCallback("shift + f", function()
{
    Console.print("Focus mode toggled");
});

// Unregister a shortcut by passing a non-function value
// Content.setKeyPressCallback("s", 0);
```
```json:testMetadata:keyboard-shortcut-navigation
{
  "testable": false,
  "skipReason": "Key press callbacks require hardware keyboard interaction"
}
```

## setName

**Signature:** `undefined setName(String newName)`
**Return Type:** `undefined`
**Call Scope:** safe
**Call Scope Note:** Simple string assignment to a member variable.
**Minimal Example:** `Content.setName("My Plugin");`

**Description:**
Sets the internal name of this Content instance.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newName | String | no | The new name for this Content instance | -- |

## setSuspendTimerCallback

**Signature:** `undefined setSuspendTimerCallback(Function suspendFunction)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Creates a WeakCallbackHolder (heap allocation).
**Minimal Example:** `Content.setSuspendTimerCallback(onSuspend);`

**Description:**
Registers a callback that is called when panel timers are suspended or resumed (e.g., when the plugin window is hidden or shown). The callback receives a single boolean argument: `true` when timers should be suspended, `false` when they should be resumed. The function must be a valid JavaScript function (validated via `HiseJavascriptEngine::isJavascriptFunction`).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| suspendFunction | Function | no | Callback receiving a boolean suspend state | Must be a JS function |

**Pitfalls:**
- [BUG] If the argument is not a valid JavaScript function, the method silently does nothing -- no error is reported and any previously registered callback remains active.

**Cross References:**
- `$API.ScriptPanel.setTimerCallback$`
- `$API.Engine.createTimerObject$`

**Example:**
```javascript:suspend-timer-callback
// Title: Handling timer suspend/resume notifications
Content.makeFrontInterface(600, 300);

inline function onSuspendChanged(isSuspended)
{
    Console.print("Timers suspended: " + isSuspended);
};

Content.setSuspendTimerCallback(onSuspendChanged);
```
```json:testMetadata:suspend-timer-callback
{
  "testable": false,
  "skipReason": "Suspend callback requires window hide/show interaction"
}
```

## setUseHighResolutionForPanels

**Signature:** `undefined setUseHighResolutionForPanels(Integer shouldUseDoubleResolution)`
**Return Type:** `undefined`
**Call Scope:** safe
**Call Scope Note:** Simple boolean assignment to a member variable.
**Minimal Example:** `Content.setUseHighResolutionForPanels(true);`

**Description:**
Enables or disables double-resolution rendering for ScriptPanel paint routines. When enabled, panels render at 2x resolution for sharper graphics on high-DPI displays.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shouldUseDoubleResolution | Integer | no | True to enable 2x resolution for panels | Boolean |

**Cross References:**
- `$API.ScriptPanel.setPaintRoutine$`

**Example:**
```javascript:hidpi-panel-rendering
// Title: Enabling HiDPI rendering for ScriptPanels
// Context: Call this immediately after makeFrontInterface if any ScriptPanel
// in your project uses a custom paint routine. Without it, panels render
// at 1x resolution and appear blurry on Retina/HiDPI displays.

Content.makeFrontInterface(900, 600);
Content.setUseHighResolutionForPanels(true);

// Now all ScriptPanels render at 2x resolution
const var panel = Content.addPanel("WaveformPanel", 10, 10);
panel.set("width", 400);
panel.set("height", 200);

panel.setPaintRoutine(function(g)
{
    // This will render at 2x resolution for crisp edges
    g.setColour(0xFF445566);
    g.fillRoundedRectangle(this.getLocalBounds(0), 4.0);
});
```
```json:testMetadata:hidpi-panel-rendering
{
  "testable": false,
  "skipReason": "Paint routine rendering is visual-only and cannot be verified via REPL"
}
```

## setValuePopupData

**Signature:** `undefined setValuePopupData(JSON jsonData)`
**Return Type:** `undefined`
**Call Scope:** safe
**Call Scope Note:** Simple var assignment to a member variable.
**Minimal Example:** `Content.setValuePopupData({"fontName": "Arial", "fontSize": 16.0});`

**Description:**
Configures the global appearance of value popups that appear when interacting with knobs and sliders. The JSON object is stored and consumed by the value popup rendering system. This applies globally to all components in this Content instance.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| jsonData | JSON | no | Configuration object for value popup appearance | See schema below |

**Cross References:**
- `$API.ScriptSlider.showControl$`

**Example:**
```javascript:example
// Title: Example
Content.setValuePopupData({
    "itemColour":   Colours.forestgreen,    // BG colour TOP
    "itemColour2":  Colours.firebrick, // BG colour BOTTOM
    "bgColour":     Colours.gainsboro,      // In fact the Border colour...
    "borderSize":   6.66,
    "textColour":   Colours.navajowhite,
    "fontSize":     66.6,
    "fontName":     "Comic Sans MS"
});
```

**Example:**
```javascript:example
// Title: Example
Content.setValuePopupData({
    "fontName":"Comic Sans MS",
    "fontSize": 14,
    "borderSize": 1,
    "borderRadius": 1,
    "margin":0,
    "bgColour": 0xFF636363,
    "itemColour": 0xFF000000,
    "itemColour2": 0xFF000000,
     "textColour": 0xFF636363 
});
```

## setWidth

**Signature:** `undefined setWidth(int newWidth)`
**Return Type:** `undefined`
**Call Scope:** safe
**Call Scope Note:** Simple integer comparison and assignment. The async broadcast is dispatched but does not block.
**Minimal Example:** `Content.setWidth(900);`

**Description:**
Sets the interface width in pixels. Only broadcasts a size change if the new value differs from the current width AND the height is non-zero (preventing spurious resize events during initial setup). Use `makeFrontInterface()` to set both dimensions at once.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newWidth | Integer | no | Interface width in pixels | > 0 |

**Pitfalls:**
- If `setWidth` is called before `setHeight` (height is still the default 50, which is non-zero), it will broadcast. But if height were 0, no broadcast occurs.

**Cross References:**
- `$API.Content.setHeight$`
- `$API.Content.makeFrontInterface$`
- `$API.Content.getInterfaceSize$`

## showModalTextInput

**Signature:** `undefined showModalTextInput(JSON properties, Function callback)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Creates a TextInputData object (heap allocation) and broadcasts asynchronously to the message thread.
**Minimal Example:** `Content.showModalTextInput({"text": "Enter name"}, onTextInput);`

**Description:**
Shows a modal text input overlay on the interface. The properties JSON configures the text editor's position, size, colours, font, and initial text. The callback is called when the user presses Return (ok=true), Escape (ok=false), or the editor loses focus (ok=false). The text editor is automatically removed after the callback fires.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| properties | JSON | no | Configuration for the text input overlay | See schema below |
| callback | Function | no | Called with (ok, text) when input is dismissed | Must have 2 args |

**Pitfalls:**
- The callback uses a closure with a capture list (`function [knob](ok, input)`) because inner anonymous functions cannot access outer function parameters. Without the capture, the component reference would be undefined inside the callback.

**Cross References:**
- `$API.ScriptPanel.showAsPopup$`
- `$API.Engine.getValueForText$`

**Example:**
```javascript:example
// Title: Example
// Attaches a text input box to the given panel.
inline function make(n)
{
	local p = Content.getComponent(n);
	
	p.set("allowCallbacks", "Clicks Only");
	
	p.setPaintRoutine(function(g)
	{
		g.setColour(Colours.white);
		g.drawText(this.getValue(), this.getLocalBounds(0));
	});
	
	p.setMouseCallback(function(event)
	{
		if(event.clicked && event.shiftDown)
		{
			// Since we want to pass that into the textbox callback as lambda ;
			// we need to store it as local variable before.
			var tp = this;
			
			var textBoxProperties = {
				parentComponent: this.get("id"),
				x: 10,
				y: 10,
				width: 80,
				height: 20,
				bgColour: Colours.red,
				textColour: Colours.black
			};
			
			Content.showModalTextInput(textBoxProperties, function[tp](status, value)
			{
				tp.setValue(parseInt(value));
				tp.changed();
				tp.repaint();
			});
		}
	});
	
	return p;
}

// Call this multiple times. If you click on the second panel while the
// first input box is active you'll see that it works exclusively.
make("Panel1");
make("Panel2");
```

## setToolbarProperties

(Disabled -- deprecated since 2017. Always throws a script error.)

## addAudioWaveform

**Signature:** `ScriptAudioWaveform addAudioWaveform(String audioWaveformName, int x, int y)`
**Return Type:** `ScriptAudioWaveform`
**Call Scope:** init
**Minimal Example:** `var wf = Content.addAudioWaveform("Waveform1", 10, 10);`

**Description:**
Creates a ScriptAudioWaveform component (an audio waveform display for visualising audio files or buffers) and adds it to the interface. Accepts either 1 argument (name only, position defaults to 0,0) or 3 arguments (name, x, y). If a component with the same name already exists, the existing component is returned and its position is optionally updated. This idempotent behavior makes the call safe across recompiles. Can only be called during onInit -- calling after initialization throws a script error.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| audioWaveformName | String | no | Unique name for the component (also used as its ID) | No whitespace |
| x | Integer | no | Horizontal position in pixels | >= 0 (clamped via jmax) |
| y | Integer | no | Vertical position in pixels | >= 0 (clamped via jmax) |

**Pitfalls:**
- Calling this method after onInit throws "Tried to add a component after onInit()". All component creation must happen during onInit.

**Cross References:**
- `$API.Content.getComponent$`
- `$API.Content.componentExists$`

## setUpdateExistingPosition

**Signature:** `void setUpdateExistingPosition(bool shouldUpdateExistingComponents)`
**Return Type:** `void`
**Call Scope:** safe
**Minimal Example:** `Content.setUpdateExistingPosition(false);`

**Description:**
Controls whether re-calling `addXXX()` on an already-existing component updates that component's x/y position. Defaults to `true`, meaning `Content.addButton("Btn1", newX, newY)` will move the existing "Btn1" to (newX, newY) on recompile. Setting this to `false` prevents position updates, which is useful when component positions are managed dynamically at runtime (e.g., via `setPropertiesFromJSON` or layout scripts) and should not be reset on recompile.

The flag is checked inside the `addComponent<T>()` template: `if ((x != -1 && y != -1) && updateExistingPositions)`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shouldUpdateExistingComponents | bool | no | Whether addXXX() calls update position of existing components | Default: true |

**Cross References:**
- All `addXXX()` component creation methods (addKnob, addButton, addPanel, etc.)
- `$API.Content.setPropertiesFromJSON$`

**Example:**
```javascript:disable-position-update
// Title: Preventing addXXX from resetting dynamic layouts
Content.makeFrontInterface(900, 600);

// Disable position updates -- layout is managed by script
Content.setUpdateExistingPosition(false);

const var btn = Content.addButton("DynBtn", 10, 10);
// On recompile, "DynBtn" will NOT be moved back to (10, 10)
// if it was repositioned at runtime
```
```json:testMetadata:disable-position-update
{
  "testable": true,
  "verifyScript": {"type": "REPL", "expression": "isDefined(btn)", "value": true}
}
```