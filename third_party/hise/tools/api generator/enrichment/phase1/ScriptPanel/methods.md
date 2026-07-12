## addChildPanel

**Signature:** `ScriptPanel addChildPanel()`
**Return Type:** `ScriptPanel`
**Call Scope:** unsafe
**Minimal Example:** `var child = {obj}.addChildPanel();`

**Description:**
Creates a new ScriptPanel as a child of this panel and returns it. The child panel is a full ScriptPanel instance with its own paint routine, mouse callbacks, timers, and `data` object. Child panels are added to the parent's internal child panel list.

**Cross References:**
- `$API.ScriptPanel.getChildPanelList$`
- `$API.ScriptPanel.getParentPanel$`
- `$API.ScriptPanel.removeFromParent$`

**Example:**
```javascript:child-panel-setup
// Title: Creating and configuring a child panel
const var pnl = Content.addPanel("ParentPanel", 0, 0);
pnl.set("width", 300);
pnl.set("height", 200);

var child = pnl.addChildPanel();
child.set("width", 100);
child.set("height", 50);
child.set("x", 10);
child.set("y", 10);

child.setPaintRoutine(function(g)
{
    g.fillAll(0xFF444444);
});

child.repaint();
```
```json:testMetadata:child-panel-setup
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "pnl.getChildPanelList().length", "value": 1},
    {"type": "REPL", "expression": "child.getWidth()", "value": 100}
  ]
}
```

---

## addToMacroControl

**Signature:** `undefined addToMacroControl(Integer macroIndex)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.addToMacroControl(0);`

**Description:**
Assigns this component to a macro controller slot. Sets the internal `connectedMacroIndex`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| macroIndex | Integer | no | The macro controller index | 0-7 |

---

## closeAsPopup

**Signature:** `undefined closeAsPopup()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.closeAsPopup();`

**Description:**
Hides this panel if it is currently shown as a popup overlay. Sets the internal `shownAsPopup` flag to false and removes the panel from the Content's popup panel list. Has no effect if the panel is not currently shown as a popup.

**Cross References:**
- `$API.ScriptPanel.showAsPopup$`
- `$API.ScriptPanel.isVisibleAsPopup$`
- `$API.ScriptPanel.setIsModalPopup$`

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
- `$API.ScriptComponent.showControl$`

---

## get

**Signature:** `var get(String propertyName)`
**Return Type:** `var`
**Call Scope:** safe
**Minimal Example:** `var v = {obj}.get("text");`

**Description:**
Returns the current value of the named property. Reports a script error if the property does not exist. Base properties available on all components: `text`, `visible`, `enabled`, `locked`, `x`, `y`, `width`, `height`, `min`, `max`, `defaultValue`, `tooltip`, `bgColour`, `itemColour`, `itemColour2`, `textColour`, `macroControl`, `saveInPreset`, `isPluginParameter`, `pluginParameterName`, `pluginParameterGroup`, `deferControlCallback`, `isMetaParameter`, `linkedTo`, `automationID`, `useUndoManager`, `parentComponent`, `processorId`, `parameterId`. ScriptPanel adds: `borderSize`, `borderRadius`, `opaque`, `allowDragging`, `allowCallbacks`, `popupMenuItems`, `popupOnRightClick`, `popupMenuAlign`, `selectedPopupIndex`, `stepSize`, `enableMidiLearn`, `holdIsRightClick`, `isPopupPanel`, `bufferToImage`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| propertyName | String | no | The name of a component property to retrieve | Must be a valid property ID for this component type |

**Cross References:**
- `$API.ScriptComponent.set$`
- `$API.ScriptComponent.getAllProperties$`

---

## getAllProperties

**Signature:** `Array getAllProperties()`
**Return Type:** `Array`
**Call Scope:** unsafe
**Minimal Example:** `var props = {obj}.getAllProperties();`

**Description:**
Returns an array of strings containing all active (non-deactivated) property IDs for this component. Includes both base ScriptComponent properties and ScriptPanel-specific properties.

**Cross References:**
- `$API.ScriptComponent.get$`
- `$API.ScriptComponent.set$`

---

## getAnimationData

**Signature:** `JSON getAnimationData()`
**Return Type:** `JSON`
**Call Scope:** unsafe
**Minimal Example:** `var anim = {obj}.getAnimationData();`

**Description:**
Returns a JSON object describing the current Lottie animation state. Requires `HISE_INCLUDE_RLOTTIE` to be enabled. If no animation is loaded or rLottie is not available, the returned object will have `active` set to false.

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| active | bool | Whether an animation is loaded and valid |
| currentFrame | int | Current frame number |
| numFrames | int | Total number of frames in the animation |
| frameRate | int | Animation frame rate (frames per second) |

**Cross References:**
- `$API.ScriptPanel.setAnimation$`
- `$API.ScriptPanel.setAnimationFrame$`

---

## getChildComponents

**Signature:** `Array getChildComponents()`
**Return Type:** `Array`
**Call Scope:** unsafe
**Minimal Example:** `var children = {obj}.getChildComponents();`

**Description:**
Returns an array of ScriptComponent references for all child components (components whose `parentComponent` property is set to this component). Does not include the component itself in the result.

**Cross References:**
- `$API.ScriptPanel.getChildPanelList$`

---

## getChildPanelList

**Signature:** `Array getChildPanelList()`
**Return Type:** `Array`
**Call Scope:** unsafe
**Minimal Example:** `var children = {obj}.getChildPanelList();`

**Description:**
Returns an array of ScriptPanel references for all child panels created via `addChildPanel()`. This is separate from `getChildComponents()` which returns components parented via the `parentComponent` property. Child panels are ScriptPanel instances within this panel's internal child panel hierarchy.

**Cross References:**
- `$API.ScriptPanel.addChildPanel$`
- `$API.ScriptPanel.getParentPanel$`
- `$API.ScriptComponent.getChildComponents$`

---

## getGlobalPositionX

**Signature:** `Integer getGlobalPositionX()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var x = {obj}.getGlobalPositionX();`

**Description:**
Returns the absolute x-position relative to the interface root, computed by recursively adding parent component x-offsets.

**Cross References:**
- `$API.ScriptComponent.getGlobalPositionY$`

---

## getGlobalPositionY

**Signature:** `Integer getGlobalPositionY()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var y = {obj}.getGlobalPositionY();`

**Description:**
Returns the absolute y-position relative to the interface root, computed by recursively adding parent component y-offsets.

**Cross References:**
- `$API.ScriptComponent.getGlobalPositionX$`

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
Returns the component's ID as a string (the variable name used when creating the component, e.g. "Panel1").

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

## getParentPanel

**Signature:** `ScriptPanel getParentPanel()`
**Return Type:** `ScriptPanel`
**Call Scope:** safe
**Minimal Example:** `var parent = {obj}.getParentPanel();`

**Description:**
Returns the parent ScriptPanel if this panel was created via `addChildPanel()`, or `undefined` if this is a top-level panel. This is specific to the child panel hierarchy, not the `parentComponent` property system.

**Cross References:**
- `$API.ScriptPanel.addChildPanel$`
- `$API.ScriptPanel.getChildPanelList$`
- `$API.ScriptPanel.removeFromParent$`

---

## getValue

**Signature:** `var getValue()`
**Return Type:** `var`
**Call Scope:** safe
**Minimal Example:** `var v = {obj}.getValue();`

**Description:**
Returns the current value of the component. Uses a `SimpleReadWriteLock` for thread-safe read access.

**Pitfalls:**
- The stored value must not be a String. If it is, an assertion fires in debug builds.

**Cross References:**
- `$API.ScriptComponent.setValue$`
- `$API.ScriptComponent.getValueNormalized$`

---

## getValueNormalized

**Disabled:** redundant
**Disabled Reason:** Base implementation simply returns `getValue()` directly. Only meaningful on ScriptSlider, which maps the actual value back to a 0..1 range using its configured mode.

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
- `$API.ScriptComponent.loseFocus$`

---

## isImageLoaded

**Signature:** `Integer isImageLoaded(String prettyName)`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var loaded = {obj}.isImageLoaded("myImage");`

**Description:**
Returns true (1) if an image with the given pretty name has been loaded via `loadImage()`, false (0) otherwise. Checks the internal loaded images list for a matching name.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| prettyName | String | no | The pretty name assigned during loadImage() | -- |

**Cross References:**
- `$API.ScriptPanel.loadImage$`
- `$API.ScriptPanel.unloadAllImages$`

---

## isVisibleAsPopup

**Signature:** `Integer isVisibleAsPopup()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var visible = {obj}.isVisibleAsPopup();`

**Description:**
Returns true (1) if this panel is currently shown as a popup overlay, false (0) otherwise.

**Cross References:**
- `$API.ScriptPanel.showAsPopup$`
- `$API.ScriptPanel.closeAsPopup$`

---

## loadImage

**Signature:** `undefined loadImage(String imageName, String prettyName)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.loadImage("{PROJECT_FOLDER}myImage.png", "myImage");`

**Description:**
Loads an image from the project's Images pool and stores it with the given pretty name alias. Images are loaded via the expansion handler, so they can come from the main project or from expansion packs. Use the pretty name to reference the image later in paint routines or with `setImage()`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| imageName | String | no | The image file path, typically using `{PROJECT_FOLDER}` prefix | Must be a valid image resource path |
| prettyName | String | no | An alias for referencing this image later | Should be unique among loaded images |

**Cross References:**
- `$API.ScriptPanel.isImageLoaded$`
- `$API.ScriptPanel.unloadAllImages$`
- `$API.ScriptPanel.setImage$`

---

## loseFocus

**Signature:** `undefined loseFocus()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.loseFocus();`

**Description:**
Notifies all z-level listeners that the component wants to lose keyboard focus. Triggers the `wantsToLoseFocus()` callback on all registered `ZLevelListener` instances.

**Cross References:**
- `$API.ScriptComponent.grabFocus$`

---

## removeFromParent

**Signature:** `Integer removeFromParent()`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Minimal Example:** `var success = {obj}.removeFromParent();`

**Description:**
Removes this panel from its parent panel's child list. Returns true (1) if the panel was successfully removed, false (0) if it has no parent or removal failed. Only applies to panels created via `addChildPanel()`.

**Cross References:**
- `$API.ScriptPanel.addChildPanel$`
- `$API.ScriptPanel.getParentPanel$`
- `$API.ScriptPanel.getChildPanelList$`

---

## repaint

**Signature:** `undefined repaint()`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.repaint();`

**Description:**
Schedules an asynchronous repaint of this panel. Safe to call from any thread -- the method checks the current thread and dispatches appropriately. On the scripting, sample loading, or message thread, the paint job is scheduled directly. From other threads (e.g. audio thread), it is deferred via the JavaScript thread pool. The actual paint routine executes on the scripting thread via a low-priority callback.

**Cross References:**
- `$API.ScriptPanel.setPaintRoutine$`
- `$API.ScriptPanel.repaintImmediately$`

---

## repaintImmediately

**Signature:** `undefined repaintImmediately()`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.repaintImmediately();`

**Description:**
Schedules a repaint. Despite the name, this method currently behaves identically to `repaint()` -- it schedules an asynchronous repaint rather than performing a synchronous one. The synchronous paint path was removed in a previous update.

**Pitfalls:**
- Despite the name suggesting synchronous behavior, this is identical to `repaint()` and schedules an asynchronous paint. Do not rely on the paint having completed after this call returns.

**Cross References:**
- `$API.ScriptPanel.repaint$`
- `$API.ScriptPanel.setPaintRoutine$`

---

## set

**Signature:** `undefined set(String propertyName, NotUndefined value)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.set("text", "Hello");`

**Description:**
Sets a component property to the given value. Reports a script error if the property does not exist. During `onInit`, changes are applied without UI notification; outside `onInit`, sends change notifications to update the UI. Base properties available on all components: `text`, `visible`, `enabled`, `locked`, `x`, `y`, `width`, `height`, `min`, `max`, `defaultValue`, `tooltip`, `bgColour`, `itemColour`, `itemColour2`, `textColour`, `macroControl`, `saveInPreset`, `isPluginParameter`, `pluginParameterName`, `pluginParameterGroup`, `deferControlCallback`, `isMetaParameter`, `linkedTo`, `automationID`, `useUndoManager`, `parentComponent`, `processorId`, `parameterId`. ScriptPanel adds: `borderSize`, `borderRadius`, `opaque`, `allowDragging`, `allowCallbacks`, `popupMenuItems`, `popupOnRightClick`, `popupMenuAlign`, `selectedPopupIndex`, `stepSize`, `enableMidiLearn`, `holdIsRightClick`, `isPopupPanel`, `bufferToImage`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| propertyName | String | no | The property identifier to set | Must be a valid property ID for this component type |
| value | NotUndefined | no | The new value for the property | Type must match the property's expected type |

**Cross References:**
- `$API.ScriptComponent.get$`
- `$API.ScriptComponent.getAllProperties$`

---

## setAnimation

**Signature:** `undefined setAnimation(String base64LottieAnimation)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setAnimation(lottieBase64);`

**Description:**
Loads a Lottie animation from a base64-encoded JSON string. Requires `HISE_INCLUDE_RLOTTIE` to be enabled. The animation is sized to match the panel dimensions with a 2x scale factor. After loading, use `setAnimationFrame()` to render specific frames and `getAnimationData()` to query animation state.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| base64LottieAnimation | String | no | A base64-encoded Lottie JSON animation string | Must be valid base64-encoded Lottie JSON |

**Cross References:**
- `$API.ScriptPanel.setAnimationFrame$`
- `$API.ScriptPanel.getAnimationData$`

---

## setAnimationFrame

**Signature:** `undefined setAnimationFrame(Integer numFrame)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setAnimationFrame(0);`

**Description:**
Renders the specified frame of the loaded Lottie animation. Requires `HISE_INCLUDE_RLOTTIE` and a prior call to `setAnimation()`. Updates the animation data and flushes the draw handler to display the frame.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| numFrame | Integer | no | The frame number to render | 0 to numFrames-1 |

**Cross References:**
- `$API.ScriptPanel.setAnimation$`
- `$API.ScriptPanel.getAnimationData$`

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

When passing individual key descriptions, each can be a string using JUCE key description format (e.g. "A", "ctrl + S", "F5", "shift + tab") or a JSON object.

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
- `$API.ScriptComponent.setKeyPressCallback$`

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

**Cross References:**
- `$API.ScriptComponent.changed$`

---

## setDraggingBounds

**Signature:** `undefined setDraggingBounds(Array area)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setDraggingBounds([0, 0, 500, 500]);`

**Description:**
Sets the rectangular area that constrains this panel's drag movement when `allowDragging` is enabled. The array specifies `[x, y, width, height]` in pixels. The panel cannot be dragged outside these bounds.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| area | Array | no | Bounding rectangle as [x, y, width, height] | Array with 4 numeric elements |

**Cross References:**
- `$API.ScriptPanel.startInternalDrag$`

---

## setFileDropCallback

**Signature:** `undefined setFileDropCallback(String callbackLevel, String wildcard, Function dropFunction)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setFileDropCallback("Drop Only", "*.wav", onFileDrop);`

**Description:**
Registers a callback for file drag-and-drop events on this panel. The callback level controls which events fire, and the wildcard filters accepted file types.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| callbackLevel | String | yes | The file drop callback level | See value descriptions |
| wildcard | String | yes | File extension filter | e.g. "*.wav;*.aif" |
| dropFunction | Function | yes | Callback function receiving file drop information | Must be an inline function with 1 parameter |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| "No Callbacks" | No file drop callbacks |
| "Drop Only" | Only fired when files are dropped |
| "Drop & Hover" | Drop + hover events while dragging over the panel |
| "All Callbacks" | All file drag events |

**Callback Signature:** dropFunction(dropInfo: Object)

**Cross References:**
- `$API.ScriptPanel.startExternalFileDrag$`

**Example:**
```javascript:file-drop-handler
// Title: Basic file drop handler for audio files
const var pnl = Content.addPanel("DropPanel", 0, 0);
pnl.set("width", 200);
pnl.set("height", 100);

inline function onFileDrop(info)
{
    Console.print("File dropped");
};

pnl.setFileDropCallback("Drop Only", "*.wav;*.aif", onFileDrop);
```
```json:testMetadata:file-drop-handler
{
  "testable": false,
  "skipReason": "Requires external file drag interaction"
}
```

---

## setImage

**Signature:** `undefined setImage(String imageName, Integer xOffset, Integer yOffset)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setImage("filmstrip", 0, 0);`

**Description:**
Switches the panel to fixed image mode, bypassing the paint routine entirely. Clips a region from a previously loaded image (via `loadImage()`) and renders it as a single draw action. This is used for filmstrip-style rendering where different offsets show different frames. Either the x or y offset must be 0 -- the other dimension is calculated from the panel's aspect ratio.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| imageName | String | no | The pretty name of a previously loaded image | Must have been loaded via loadImage() |
| xOffset | Integer | no | Horizontal pixel offset into the image | One of xOffset or yOffset must be 0 |
| yOffset | Integer | no | Vertical pixel offset into the image | One of xOffset or yOffset must be 0 |

**Pitfalls:**
- Calling setImage() clears any previously set paint routine. The panel switches to fixed image mode until a new paint routine is set via setPaintRoutine().
- Either xOffset or yOffset must be 0. The non-zero offset selects the frame along the strip direction.

**Cross References:**
- `$API.ScriptPanel.loadImage$`
- `$API.ScriptPanel.setPaintRoutine$`

---

## setIsModalPopup

**Signature:** `undefined setIsModalPopup(Integer shouldBeModal)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setIsModalPopup(1);`

**Description:**
Sets whether this popup panel displays with a dark modal background overlay. When modal, the background behind the popup is dimmed and clicks outside the popup close it. Only relevant for panels used as popups via `showAsPopup()`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shouldBeModal | Integer | no | Whether the popup should be modal | 1 = modal, 0 = non-modal |

**Cross References:**
- `$API.ScriptPanel.showAsPopup$`
- `$API.ScriptPanel.closeAsPopup$`

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
| hasFocus | bool | true if gained focus, false if lost |

**Pitfalls:**
- MUST call `setConsumedKeyPresses()` BEFORE calling this method. Reports a script error otherwise.

**Cross References:**
- `$API.ScriptComponent.setConsumedKeyPresses$`

---

## setLoadingCallback

**Signature:** `undefined setLoadingCallback(Function loadingFunction)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setLoadingCallback(onPreloadState);`

**Description:**
Registers a callback that fires when sample preloading starts or finishes. The callback receives a boolean parameter: true when loading starts, false when loading completes. Registers this panel as a `PreloadListener` on the SampleManager. Passing a non-function value removes the listener.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| loadingFunction | Function | yes | Callback function, or non-function to remove | Must be an inline function with 1 parameter |

**Callback Signature:** loadingFunction(isPreloading: bool)

**Example:**
```javascript:loading-callback
// Title: Show loading state during sample preloading
const var pnl = Content.addPanel("LoadingOverlay", 0, 0);
pnl.set("width", 200);
pnl.set("height", 100);
pnl.set("visible", false);

inline function onPreloadState(isPreloading)
{
    pnl.set("visible", isPreloading);
};

pnl.setLoadingCallback(onPreloadState);
```
```json:testMetadata:loading-callback
{
  "testable": false,
  "skipReason": "Requires sample preloading trigger which cannot be scripted"
}
```

---

## setLocalLookAndFeel

**Signature:** `undefined setLocalLookAndFeel(ScriptObject lafObject)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setLocalLookAndFeel(laf);`

**Description:**
Attaches a scripted look and feel object to this component and all its children. Pass `undefined` to clear it. The object must be a `ScriptedLookAndFeel` instance.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| lafObject | ScriptObject | no | A ScriptedLookAndFeel object, or undefined to clear | Must be a ScriptedLookAndFeel instance |

**Pitfalls:**
- Propagates to ALL child components automatically.
- If the LAF uses CSS (has a stylesheet), automatically calls `setStyleSheetClass({})` to initialize the class selector.

**Cross References:**
- `$API.ScriptComponent.setStyleSheetClass$`
- `$API.ScriptComponent.setStyleSheetProperty$`
- `$API.ScriptComponent.setStyleSheetPseudoState$`

---

## setMouseCallback

**Signature:** `undefined setMouseCallback(Function mouseFunction)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setMouseCallback(onMouse);`

**Description:**
Registers a callback for mouse events on this panel. The `allowCallbacks` property must be set to a level that enables the desired event types (default is "No Callbacks"). The callback receives a JSON event object with positional data, state flags, modifier keys, and popup menu results.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| mouseFunction | Function | yes | Callback function receiving mouse event object | Must be an inline function with 1 parameter |

**Callback Signature:** mouseFunction(event: Object)

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| mouseDownX | int | X position of initial mouse down |
| mouseDownY | int | Y position of initial mouse down |
| x | int | Current x position |
| y | int | Current y position |
| clicked | bool | True on mouse down |
| doubleClick | bool | True on double click |
| rightClick | bool | True if right button |
| mouseUp | bool | True on mouse up |
| drag | bool | True during drag |
| isDragOnly | bool | True if was only a drag (no click callback) |
| dragX | int | Horizontal drag distance |
| dragY | int | Vertical drag distance |
| insideDrag | bool | True if drag is inside component bounds |
| hover | bool | True on hover (requires "Clicks & Hover" or above) |
| result | int | Popup menu result (selected item index) |
| itemText | String | Popup menu selected item text |
| shiftDown | bool | True if Shift held |
| cmdDown | bool | True if Cmd/Ctrl held |
| altDown | bool | True if Alt held |
| ctrlDown | bool | True if Ctrl held |

**Pitfalls:**
- The `allowCallbacks` property must be set to an appropriate level before the mouse callback will fire. The default is "No Callbacks", so calling `setMouseCallback` alone does nothing.

**Cross References:**
- `$API.ScriptPanel.setPaintRoutine$`
- `$API.ScriptPanel.setDraggingBounds$`
- `$API.ScriptPanel.setMouseCursor$`

**Example:**
```javascript:mouse-callback-clicks
// Title: Mouse click handler with visual feedback
const var pnl = Content.addPanel("ClickPanel", 0, 0);
pnl.set("width", 200);
pnl.set("height", 100);
pnl.set("allowCallbacks", "Clicks Only");

pnl.data.clickCount = 0;

inline function onMouse(event)
{
    if (event.clicked)
    {
        this.data.clickCount++;
        this.repaint();
    }
};

pnl.setMouseCallback(onMouse);

pnl.setPaintRoutine(function(g)
{
    g.fillAll(0xFF333333);
    g.setColour(0xFFFFFFFF);
    g.drawAlignedText("Clicks: " + this.data.clickCount, [0, 0, this.getWidth(), this.getHeight()], "centred");
});

pnl.repaint();

// --- test-only ---
Console.testCallback(pnl, "setMouseCallback", {
    "clicked": true, "x": 100, "y": 50,
    "mouseDownX": 100, "mouseDownY": 50,
    "mouseUp": false, "doubleClick": false, "rightClick": false
});
// --- end test-only ---
```
```json:testMetadata:mouse-callback-clicks
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "pnl.data.clickCount", "value": 1}
  ]
}
```

---

## setMouseCursor

**Signature:** `undefined setMouseCursor(var pathOrName, Integer colour, Array hitPoint)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setMouseCursor("PointingHandCursor", 0, [0, 0]);`

**Description:**
Sets the mouse cursor for this panel. Accepts either a Path object for a custom cursor or a string for a standard JUCE cursor type. When using a Path, the colour parameter tints the path and hitPoint specifies the cursor hotspot as normalized [x, y] coordinates (0-1 range).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| pathOrName | var | no | A Path object for custom cursor, or a String for standard cursor | See value descriptions for valid string names |
| colour | Integer | no | Colour tint for path cursors (ARGB hex) | Only used with Path; ignored for string cursors |
| hitPoint | Array | no | Cursor hotspot as [x, y] normalized coordinates | Values 0.0-1.0; only used with Path |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| "ParentCursor" | Inherits cursor from parent component |
| "NoCursor" | Hides the cursor |
| "NormalCursor" | Standard arrow cursor |
| "WaitCursor" | Hourglass/spinner cursor |
| "IBeamCursor" | Text editing cursor |
| "CrosshairCursor" | Crosshair cursor |
| "CopyingCursor" | Copy operation cursor |
| "PointingHandCursor" | Hand pointing cursor |
| "DraggingHandCursor" | Grabbing hand cursor |
| "LeftRightResizeCursor" | Horizontal resize cursor |
| "UpDownResizeCursor" | Vertical resize cursor |
| "UpDownLeftRightResizeCursor" | Four-way resize cursor |
| "TopEdgeResizeCursor" | Top edge resize cursor |
| "BottomEdgeResizeCursor" | Bottom edge resize cursor |
| "LeftEdgeResizeCursor" | Left edge resize cursor |
| "RightEdgeResizeCursor" | Right edge resize cursor |
| "TopLeftCornerResizeCursor" | Top-left corner resize cursor |
| "TopRightCornerResizeCursor" | Top-right corner resize cursor |
| "BottomLeftCornerResizeCursor" | Bottom-left corner resize cursor |
| "BottomRightCornerResizeCursor" | Bottom-right corner resize cursor |

**Cross References:**
- `$API.ScriptPanel.setMouseCallback$`

---

## setPaintRoutine

**Signature:** `undefined setPaintRoutine(Function paintFunction)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setPaintRoutine(function(g){ g.fillAll(0xFF222222); });`

**Description:**
Registers a paint function that receives a Graphics object for custom drawing. The function is called when `repaint()` is invoked. The paint routine executes on the scripting thread via a low-priority job, not on the calling thread. Canvas resolution accounts for high-DPI settings and the global scale factor (capped at 2x).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| paintFunction | Function | no | A function receiving a Graphics object | Must have exactly 1 parameter |

**Callback Signature:** paintFunction(g: Graphics)

**Pitfalls:**
- Calling `setImage()` clears the paint routine. Conversely, setting a new paint routine cancels fixed image mode.

**Cross References:**
- `$API.ScriptPanel.repaint$`
- `$API.ScriptPanel.repaintImmediately$`
- `$API.ScriptPanel.setImage$`

**Example:**
```javascript:paint-routine-basics
// Title: Basic paint routine with text
const var pnl = Content.addPanel("PaintPanel", 0, 0);
pnl.set("width", 200);
pnl.set("height", 100);

pnl.setPaintRoutine(function(g)
{
    g.fillAll(0xFF222222);
    g.setColour(0xFFFFFFFF);
    g.setFont("Arial", 16.0);
    g.drawAlignedText("Hello Panel", [0, 0, this.getWidth(), this.getHeight()], "centred");
});

pnl.repaint();
```
```json:testMetadata:paint-routine-basics
{
  "testable": false,
  "skipReason": "Visual output cannot be verified programmatically"
}
```

---

## setPanelValueWithUndo

**Signature:** `undefined setPanelValueWithUndo(var oldValue, var newValue, String actionName)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setPanelValueWithUndo(0.0, 1.0, "Toggle");`

**Description:**
Sets the panel value with undo support, creating an undoable action. For simple numeric values, creates an `UndoableControlEvent`. For complex values (arrays, objects), creates a `PanelComplexDataUndoEvent`. Both call `setValue()` and `changed()` on perform and undo. Uses the MainController's control undo manager.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| oldValue | var | no | The current value (for undo) | -- |
| newValue | var | no | The new value to set | -- |
| actionName | String | no | Human-readable name for the undo action | -- |

**Pitfalls:**
- The `oldValue` parameter must match the current value for undo to restore correctly. It is the caller's responsibility to pass the correct current value.

**Cross References:**
- `$API.ScriptComponent.setValue$`
- `$API.ScriptComponent.setValueWithUndo$`

---

## setPopupData

**Signature:** `undefined setPopupData(JSON jsonData, Array position)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setPopupData({}, [100, 100, 300, 200]);`

**Description:**
Configures this panel as a popup by setting its FloatingTile JSON configuration and bounds. The position array specifies `[x, y, width, height]` for the popup. Call `showAsPopup()` to display the configured popup.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| jsonData | JSON | no | FloatingTile JSON configuration | -- |
| position | Array | no | Popup bounds as [x, y, width, height] | Array with 4 numeric elements |

**Cross References:**
- `$API.ScriptPanel.showAsPopup$`
- `$API.ScriptPanel.closeAsPopup$`
- `$API.ScriptPanel.setIsModalPopup$`

---

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

---

## setPropertiesFromJSON

**Disabled:** no-op
**Disabled Reason:** Not registered as a direct API method on component instances. Available on the Content object as `Content.setPropertiesFromJSON(componentName, jsonData)` instead.

---

## setStyleSheetClass

**Signature:** `undefined setStyleSheetClass(String classIds)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setStyleSheetClass(".active");`

**Description:**
Sets the CSS class selectors for this component. The component's own type class (`.scriptpanel`) is automatically prepended. Creates the `ComponentStyleSheetProperties` value tree if it does not yet exist.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| classIds | String | no | Space-separated CSS class selectors to apply | e.g. ".myClass .highlighted" |

**Cross References:**
- `$API.ScriptComponent.setStyleSheetProperty$`
- `$API.ScriptComponent.setStyleSheetPseudoState$`
- `$API.ScriptComponent.setLocalLookAndFeel$`

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
| value | NotUndefined | no | The value to assign | Type must match the conversion specified by the type parameter |
| type | String | no | The unit/type conversion to apply before storing | See value descriptions |

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
- `$API.ScriptComponent.setStyleSheetClass$`
- `$API.ScriptComponent.setStyleSheetPseudoState$`
- `$API.ScriptComponent.setLocalLookAndFeel$`

---

## setStyleSheetPseudoState

**Signature:** `undefined setStyleSheetPseudoState(String pseudoState)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setStyleSheetPseudoState(":hover");`

**Description:**
Sets one or more CSS pseudo-state selectors on this component. Multiple states can be combined in one string (e.g. ":hover:active"). Pass an empty string "" to clear all pseudo-states. Automatically calls `sendRepaintMessage()` after setting the state.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| pseudoState | String | no | One or more CSS pseudo-state selectors | Can combine multiple; pass "" to clear |

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
- `$API.ScriptComponent.setStyleSheetClass$`
- `$API.ScriptComponent.setStyleSheetProperty$`
- `$API.ScriptComponent.setLocalLookAndFeel$`

---

## setTimerCallback

**Signature:** `undefined setTimerCallback(Function timerFunction)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setTimerCallback(onTimer);`

**Description:**
Registers a callback function that fires periodically when the timer is active. The callback takes no parameters and fires on the message thread. After registering the callback, call `startTimer(ms)` to begin. The timer is automatically stopped on recompilation.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| timerFunction | Function | yes | A zero-parameter callback function | Must be an inline function with 0 parameters |

**Callback Signature:** timerFunction()

**Cross References:**
- `$API.ScriptPanel.setPaintRoutine$`
- `$API.ScriptPanel.repaint$`

**Example:**
```javascript:timer-callback-counter
// Title: Timer-driven counter with repaint
const var pnl = Content.addPanel("TimerPanel", 0, 0);
pnl.set("width", 200);
pnl.set("height", 50);

pnl.data.counter = 0;

inline function onTimer()
{
    this.data.counter++;
    this.repaint();
};

pnl.setTimerCallback(onTimer);

pnl.setPaintRoutine(function(g)
{
    g.fillAll(0xFF222222);
    g.setColour(0xFFFFFFFF);
    g.drawAlignedText("Count: " + this.data.counter, [0, 0, this.getWidth(), this.getHeight()], "centred");
});

pnl.startTimer(100);
pnl.repaint();
```
```json:testMetadata:timer-callback-counter
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "delay": 500, "expression": "pnl.data.counter > 0", "value": true}
  ]
}
```

---

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

---

## setValue

**Signature:** `undefined setValue(NotUndefined newValue)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setValue(0.5);`

**Description:**
Sets the component's value. Thread-safe -- can be called from any thread; the UI update happens asynchronously. Propagates the value to all linked component targets.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newValue | NotUndefined | no | The new value to set | Must not be a String |

**Pitfalls:**
- Do NOT pass a String value. Reports a script error.
- If called during `onInit`, the value will NOT be restored after recompilation.

**Cross References:**
- `$API.ScriptComponent.getValue$`
- `$API.ScriptComponent.setValueNormalized$`
- `$API.ScriptComponent.setValueWithUndo$`
- `$API.ScriptPanel.setPanelValueWithUndo$`

---

## setValueNormalized

**Disabled:** redundant
**Disabled Reason:** Base implementation simply calls `setValue(normalizedValue)` without any range mapping. Only meaningful on ScriptSlider, which maps the 0..1 range to the slider's actual min/max using its configured mode.

---

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
- Do NOT call this from `onControl` callbacks. It is intended for user-initiated value changes.

**Cross References:**
- `$API.ScriptComponent.setValue$`
- `$API.ScriptPanel.setPanelValueWithUndo$`

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
| zLevel | String | no | The depth level | Must be one of the four valid values (case-sensitive) |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| "Back" | Renders behind all sibling components |
| "Default" | Normal rendering order |
| "Front" | Renders in front of normal siblings |
| "AlwaysOnTop" | Always renders on top of all siblings |

---

## showAsPopup

**Signature:** `undefined showAsPopup(Integer closeOtherPopups)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.showAsPopup(1);`

**Description:**
Shows this panel as a popup overlay on top of the interface. The `isPopupPanel` property should be set to true for proper popup behavior (hidden until shown). If `closeOtherPopups` is true (1), all other currently visible popup panels are closed first.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| closeOtherPopups | Integer | no | Whether to close other visible popup panels | 1 = close others, 0 = keep others |

**Cross References:**
- `$API.ScriptPanel.closeAsPopup$`
- `$API.ScriptPanel.isVisibleAsPopup$`
- `$API.ScriptPanel.setIsModalPopup$`
- `$API.ScriptPanel.setPopupData$`

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
- `$API.ScriptComponent.fadeComponent$`

---

## startExternalFileDrag

**Signature:** `undefined startExternalFileDrag(var fileToDrag, Integer moveOriginal, Function finishCallback)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.startExternalFileDrag("C:/audio/file.wav", 0, onDragFinished);`

**Description:**
Initiates an OS-level file drag operation. Accepts a string file path, File object, or array of either. On Windows the drag operation runs synchronously; on other platforms it is deferred via `MessageManager::callAsync`. The optional finish callback is called when the drag completes.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| fileToDrag | var | no | A file path string, File object, or array of either | Must be valid file paths |
| moveOriginal | Integer | no | Whether to move (1) or copy (0) the file | 1 = move, 0 = copy |
| finishCallback | Function | no | Callback called when drag completes | -- |

**Cross References:**
- `$API.ScriptPanel.startInternalDrag$`
- `$API.ScriptPanel.setFileDropCallback$`

---

## startInternalDrag

**Signature:** `undefined startInternalDrag(var dragData)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.startInternalDrag({"type": "myDrag"});`

**Description:**
Initiates an internal HISE UI drag operation from this panel. Sends a `DragAction::Start` to the Content's RebuildListener system with the specified drag data.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| dragData | var | no | Data to associate with the drag operation | -- |

**Cross References:**
- `$API.ScriptPanel.startExternalFileDrag$`
- `$API.ScriptPanel.setDraggingBounds$`

---

## unloadAllImages

**Signature:** `undefined unloadAllImages()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.unloadAllImages();`

**Description:**
Removes all images previously loaded via `loadImage()` from this panel's image cache. After calling this, `isImageLoaded()` will return false for all previously loaded image names.

**Cross References:**
- `$API.ScriptPanel.loadImage$`
- `$API.ScriptPanel.isImageLoaded$`

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

**Cross References:**
- `$API.ScriptComponent.setValue$`
