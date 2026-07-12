## addBufferToWebSocket

**Signature:** `undefined addBufferToWebSocket(Integer bufferIndex, Buffer buffer)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.addBufferToWebSocket(0, myBuffer);`

**Description:**
Registers a buffer at a specific index for efficient repeated streaming through the websocket connection. Once registered, call `updateBuffer()` with the same index to mark the buffer as dirty and trigger a send on the next communication cycle.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| bufferIndex | Integer | no | The slot index to register the buffer at | >= 0 |
| buffer | Buffer | no | The audio buffer to register for streaming | Must be a Buffer |

**Pitfalls:**
- [BUG] Silently does nothing if the second argument is not a Buffer. No error is reported.

**Cross References:**
- `$API.ScriptWebView.updateBuffer$`
- `$API.ScriptWebView.setEnableWebSocket$`
- `$API.ScriptWebView.sendToWebSocket$`

---

## addToMacroControl

**Disabled:** property-deactivated
**Disabled Reason:** The macroControl property is deactivated for ScriptWebView. WebView components cannot be connected to macro controllers.

---

## bindCallback

**Signature:** `undefined bindCallback(String callbackId, Function functionToCall)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.bindCallback("onData", onWebViewData);`

**Description:**
Binds a HiseScript function to a JavaScript callback identifier. The JS side can then invoke this function using the Promise pattern: `callbackId(args).then(result => { ... })`. The HiseScript function receives a single argument containing the data passed from JS, and can return a value that resolves the JS Promise.

The callback is executed synchronously via `WeakCallbackHolder::callSync` -- the execution thread depends on the choc::WebView callback dispatch mechanism.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| callbackId | String | no | The identifier that JS uses to call this function | Must be unique across all bound callbacks |
| functionToCall | Function | no | A HiseScript function to execute when JS calls the identifier | Must accept 1 parameter |

**Callback Signature:** functionToCall(args: var)

**Cross References:**
- `$API.ScriptWebView.callFunction$`
- `$API.ScriptWebView.evaluate$`

**Example:**
```javascript:bind-callback-usage
// Title: Binding a HiseScript callback for JS communication
const var wv = Content.addWebView("WebView1", 0, 0);

inline function onGetData(args)
{
    return { "sampleRate": Engine.getSampleRate() };
};

wv.bindCallback("getEngineData", onGetData);
```
```json:testMetadata:bind-callback-usage
{
  "testable": false,
  "skipReason": "Requires active webview with JS content to trigger the bound callback"
}
```

---

## callFunction

**Signature:** `undefined callFunction(String javascriptFunction, NotUndefined args)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.callFunction("updateDisplay", { "value": 0.5 });`

**Description:**
Calls a JavaScript function in the global scope of the webview, passing the given arguments. The call is dispatched asynchronously to the message thread via `MessageManager::callAsync`. The function must exist in the webview's global JS scope.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| javascriptFunction | String | no | The name of the global JS function to call | Must exist in the webview's global JS scope |
| args | NotUndefined | no | Arguments to pass to the JS function | Any type; passed as-is to JS |

**Cross References:**
- `$API.ScriptWebView.bindCallback$`
- `$API.ScriptWebView.evaluate$`

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

**Cross References:**
- `$API.ScriptWebView.setControlCallback$`
- `$API.ScriptWebView.getValue$`
- `$API.ScriptWebView.setValue$`

---

## evaluate

**Signature:** `undefined evaluate(String identifier, String jsCode)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.evaluate("initTheme", "document.body.style.background = '#333';");`

**Description:**
Evaluates arbitrary JavaScript code in the webview. The call is dispatched asynchronously to the message thread. The `identifier` parameter is used as a key for persistent call tracking -- when the `enablePersistence` property is true, the code is stored and re-evaluated when new webview instances are created. Using the same identifier overwrites the previous stored code for that key.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| identifier | String | no | A unique key for persistent call tracking | Should be unique per logical evaluation; reusing a key overwrites the previous code |
| jsCode | String | no | The JavaScript code to evaluate in the webview | Must be valid JavaScript |

**Pitfalls:**
- The identifier must be unique per logical operation. Reusing an identifier overwrites the previous code stored under that key in the persistence system, which may cause unexpected behavior on webview re-initialization.

**Cross References:**
- `$API.ScriptWebView.callFunction$`
- `$API.ScriptWebView.bindCallback$`
- `$API.ScriptWebView.reset$`

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
- `$API.ScriptWebView.showControl$`

---

## get

**Signature:** `var get(String propertyName)`
**Return Type:** `var`
**Call Scope:** safe
**Minimal Example:** `var v = {obj}.get("visible");`

**Description:**
Returns the current value of the named property. Reports a script error if the property does not exist. ScriptWebView supports base properties (visible, enabled, x, y, width, height, bgColour, itemColour, itemColour2, textColour, parentComponent, etc.) plus WebView-specific properties: `enableCache`, `enablePersistence`, `scaleFactorToZoom`, `enableDebugMode`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| propertyName | String | no | The name of a component property to retrieve | Must be a valid property ID for ScriptWebView |

**Cross References:**
- `$API.ScriptWebView.set$`
- `$API.ScriptWebView.getAllProperties$`

---

## getAllProperties

**Signature:** `Array getAllProperties()`
**Return Type:** `Array`
**Call Scope:** unsafe
**Minimal Example:** `var props = {obj}.getAllProperties();`

**Description:**
Returns an array of strings containing all active (non-deactivated) property IDs for this component. Includes base ScriptComponent properties and the four WebView-specific properties (enableCache, enablePersistence, scaleFactorToZoom, enableDebugMode). Excludes deactivated properties such as macroControl, processorId, parameterId, tooltip, text, min, max, and defaultValue.

**Cross References:**
- `$API.ScriptWebView.get$`
- `$API.ScriptWebView.set$`

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

**Cross References:**
- `$API.ScriptWebView.getGlobalPositionY$`

---

## getGlobalPositionY

**Signature:** `Integer getGlobalPositionY()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var y = {obj}.getGlobalPositionY();`

**Description:**
Returns the absolute y-position relative to the interface root, computed by recursively adding parent component y-offsets.

**Cross References:**
- `$API.ScriptWebView.getGlobalPositionX$`

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
Returns the component's ID as a string (the variable name used when creating the component, e.g. "WebView1").

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
Returns the current value of the component. Uses a `SimpleReadWriteLock` for thread-safe read access.

**Cross References:**
- `$API.ScriptWebView.setValue$`

---

## getValueNormalized

**Disabled:** redundant
**Disabled Reason:** The base implementation returns getValue() directly without normalization. Only meaningful on ScriptSlider, which maps actual values to the 0..1 range.

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
- `$API.ScriptWebView.loseFocus$`

---

## loseFocus

**Signature:** `undefined loseFocus()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.loseFocus();`

**Description:**
Notifies all z-level listeners that the component wants to lose keyboard focus.

**Cross References:**
- `$API.ScriptWebView.grabFocus$`

---

## reset

**Signature:** `undefined reset()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.reset();`

**Description:**
Clears all cached resources and persistent call data from the underlying WebViewData, but preserves the file structure (root directory and index file settings). Use this to force a clean reload of web content without re-specifying the content source.

**Cross References:**
- `$API.ScriptWebView.evaluate$`
- `$API.ScriptWebView.setIndexFile$`

---

## sendRepaintMessage

**Signature:** `undefined sendRepaintMessage()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.sendRepaintMessage();`

**Description:**
Sends an asynchronous repaint message via `repaintBroadcaster`. Note that ScriptWebView manages its own rendering through the embedded browser engine, so this primarily affects the component wrapper frame rather than the web content itself.

---

## sendToWebSocket

**Signature:** `undefined sendToWebSocket(String id, NotUndefined data)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.sendToWebSocket("update", { "level": 0.8 });`

**Description:**
Sends data to the webview through the websocket connection. The data is dispatched differently based on its type:
- **String**: sent directly as a string message
- **Buffer**: sent as raw binary float data
- **JSON Object**: serialized to JSON string and sent as a string message

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| id | String | no | The message identifier | -- |
| data | NotUndefined | no | The data to send | Must be String, Buffer, or JSON Object |

**Cross References:**
- `$API.ScriptWebView.setEnableWebSocket$`
- `$API.ScriptWebView.addBufferToWebSocket$`
- `$API.ScriptWebView.setWebSocketCallback$`
- `$API.ScriptWebView.updateBuffer$`

---

## set

**Signature:** `undefined set(String propertyName, NotUndefined value)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.set("enableCache", true);`

**Description:**
Sets a component property to the given value. Reports a script error if the property does not exist. During `onInit`, changes are applied without UI notification; outside `onInit`, sends change notifications to update the UI. For the four WebView-specific properties (enableCache, enablePersistence, scaleFactorToZoom, enableDebugMode), the value is forwarded to the underlying WebViewData.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| propertyName | String | no | The property identifier to set | Must be a valid property ID for ScriptWebView |
| value | NotUndefined | no | The new value for the property | Type must match the property's expected type |

**Cross References:**
- `$API.ScriptWebView.get$`
- `$API.ScriptWebView.getAllProperties$`

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

**Pitfalls:**
- Must be called BEFORE `setKeyPressCallback`. Reports a script error if an invalid key description is provided.

**Cross References:**
- `$API.ScriptWebView.setKeyPressCallback$`

---

## setControlCallback

**Signature:** `undefined setControlCallback(Function controlFunction)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setControlCallback(onMyControl);`

**Description:**
Assigns a custom inline function as the control callback, replacing the default `onControl` handler for this component.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| controlFunction | Function | no | An inline function with 2 parameters (component, value) | Must be an inline function with exactly 2 parameters |

**Callback Signature:** controlFunction(component: ScriptComponent, value: var)

**Pitfalls:**
- The function MUST be declared with `inline function`. Regular function references are rejected with a script error.
- Must have exactly 2 parameters. Reports a script error if the parameter count is wrong.

**Cross References:**
- `$API.ScriptWebView.changed$`

---

## setEnableWebSocket

**Signature:** `undefined setEnableWebSocket(Integer port)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setEnableWebSocket(8080);`

**Description:**
Starts a TCP server on the specified port for websocket-style communication between HiseScript and the webview. Must be called before `setWebSocketCallback()` or `sendToWebSocket()`. The communication uses raw TCP with a custom framing protocol, not the standard WebSocket protocol.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| port | Integer | no | The TCP port number for the websocket server | Use -1 for a random available port |

**Cross References:**
- `$API.ScriptWebView.setWebSocketCallback$`
- `$API.ScriptWebView.sendToWebSocket$`
- `$API.ScriptWebView.addBufferToWebSocket$`

---

## setHtmlContent

**Signature:** `undefined setHtmlContent(String htmlCode)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setHtmlContent("<h1>Hello</h1>");`

**Description:**
Sets inline HTML content for the webview to render. This uses the Hardcoded content mode -- the entire page is specified as a string. For file-based content with external CSS/JS resources, use `setIndexFile()` instead.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| htmlCode | String | no | The HTML content to render | Must be valid HTML |

**Cross References:**
- `$API.ScriptWebView.setIndexFile$`
- `$API.ScriptWebView.reset$`

---

## setIndexFile

**Signature:** `undefined setIndexFile(ScriptObject indexFile)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setIndexFile(FileSystem.getFolder(FileSystem.AudioFiles).getChildFile("webview/index.html"));`

**Description:**
Sets the HTML file to be displayed by the webview. The file's parent directory becomes the root directory for resolving relative resource paths (CSS, JS, images). Requires a File object -- string paths are not accepted.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| indexFile | ScriptObject | no | A File object pointing to the HTML index file | Must be a File object (from FileSystem API) |

**Pitfalls:**
- Passing a string path instead of a File object causes a script error: "setIndexFile must be called with a file object".

**Cross References:**
- `$API.ScriptWebView.setHtmlContent$`
- `$API.ScriptWebView.reset$`

**Example:**
```javascript:set-index-file-usage
// Title: Loading a webview from an HTML file
const var wv = Content.addWebView("WebView1", 0, 0);
wv.set("width", 600);
wv.set("height", 400);

const var folder = FileSystem.getFolder(FileSystem.AudioFiles);
wv.setIndexFile(folder.getChildFile("webview/index.html"));
```
```json:testMetadata:set-index-file-usage
{
  "testable": false,
  "skipReason": "Requires an HTML file on disk at the expected path"
}
```

---

## setKeyPressCallback

**Signature:** `undefined setKeyPressCallback(Function keyboardFunction)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setKeyPressCallback(onKeyPress);`

**Description:**
Registers a callback that fires when a consumed key is pressed while this component has focus. MUST call `setConsumedKeyPresses()` BEFORE calling this method.

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
- `$API.ScriptWebView.setConsumedKeyPresses$`

---

## setLocalLookAndFeel

**Signature:** `undefined setLocalLookAndFeel(ScriptObject lafObject)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setLocalLookAndFeel(laf);`

**Description:**
Attaches a scripted look and feel object to this component and all its children. Pass false to clear it. Note that ScriptWebView renders content through an embedded browser engine, so LAF primarily affects the component wrapper rather than the web content itself.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| lafObject | ScriptObject | no | A ScriptedLookAndFeel object, or false to clear | Must be a ScriptedLookAndFeel instance |

**Pitfalls:**
- Propagates to ALL child components automatically.
- If the LAF uses CSS, automatically initializes the class selector via `setStyleSheetClass({})`.

**Cross References:**
- `$API.ScriptWebView.setStyleSheetClass$`
- `$API.ScriptWebView.setStyleSheetProperty$`
- `$API.ScriptWebView.setStyleSheetPseudoState$`

---

## setPosition

**Signature:** `undefined setPosition(Integer x, Integer y, Integer w, Integer h)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setPosition(10, 10, 600, 400);`

**Description:**
Sets the component's position and size in one call.

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
**Disabled Reason:** This method is not registered as a direct API method on component instances. Use Content.setPropertiesFromJSON() instead.

---

## setStyleSheetClass

**Signature:** `undefined setStyleSheetClass(String classIds)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setStyleSheetClass(".active");`

**Description:**
Sets the CSS class selectors for this component. The component's own type class (`.scriptwebview`) is automatically prepended.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| classIds | String | no | Space-separated CSS class selectors to apply | e.g. ".myClass .highlighted" |

**Cross References:**
- `$API.ScriptWebView.setStyleSheetProperty$`
- `$API.ScriptWebView.setStyleSheetPseudoState$`
- `$API.ScriptWebView.setLocalLookAndFeel$`

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
- `$API.ScriptWebView.setStyleSheetClass$`
- `$API.ScriptWebView.setStyleSheetPseudoState$`
- `$API.ScriptWebView.setLocalLookAndFeel$`

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
- `$API.ScriptWebView.setStyleSheetClass$`
- `$API.ScriptWebView.setStyleSheetProperty$`
- `$API.ScriptWebView.setLocalLookAndFeel$`

---

## setTooltip

**Disabled:** property-deactivated
**Disabled Reason:** The tooltip property is deactivated for ScriptWebView.

---

## setValue

**Signature:** `undefined setValue(NotUndefined newValue)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setValue(0.5);`

**Description:**
Sets the component's value. Thread-safe -- can be called from any thread; the UI update happens asynchronously. Propagates the value to all linked component targets and sends value listener messages.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newValue | NotUndefined | no | The new value to set | Must not be a String |

**Pitfalls:**
- Do NOT pass a String value. Reports a script error.
- If called during `onInit`, the value will NOT be restored after recompilation.

**Cross References:**
- `$API.ScriptWebView.getValue$`
- `$API.ScriptWebView.changed$`

---

## setValueNormalized

**Disabled:** redundant
**Disabled Reason:** The base implementation calls setValue(normalizedValue) directly without mapping. Only meaningful on ScriptSlider, which maps 0..1 to the slider's actual range.

---

## setValueWithUndo

**Disabled:** property-deactivated
**Disabled Reason:** The useUndoManager property is deactivated for ScriptWebView. Undo integration is not available for WebView components.

---

## setWebSocketCallback

**Signature:** `undefined setWebSocketCallback(Function callbackFunction)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setWebSocketCallback(onWebSocketMessage);`

**Description:**
Registers a callback function to receive incoming messages from the websocket connection. The websocket must be enabled via `setEnableWebSocket()` before calling this method, otherwise a script error is thrown.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| callbackFunction | Function | no | A function to handle incoming websocket messages | Must accept 1 parameter |

**Callback Signature:** callbackFunction(message: var)

**Pitfalls:**
- Must call `setEnableWebSocket()` before this method. Reports a script error: "You have to enable the WebSocket before calling this method".

**Cross References:**
- `$API.ScriptWebView.setEnableWebSocket$`
- `$API.ScriptWebView.sendToWebSocket$`

**Example:**
```javascript:websocket-callback-setup
// Title: Setting up websocket communication
const var wv = Content.addWebView("WebView1", 0, 0);

inline function onWebSocketMessage(message)
{
    Console.print("Received: " + message);
};

wv.setEnableWebSocket(8080);
wv.setWebSocketCallback(onWebSocketMessage);
```
```json:testMetadata:websocket-callback-setup
{
  "testable": false,
  "skipReason": "Requires active websocket connection with a client sending messages"
}
```

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

**Cross References:**
- `$API.ScriptWebView.fadeComponent$`

---

## updateBuffer

**Signature:** `undefined updateBuffer(Integer bufferIndex)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.updateBuffer(0);`

**Description:**
Marks the buffer at the given index as dirty so it will be sent to the webview on the next websocket communication cycle. The buffer must have been previously registered via `addBufferToWebSocket()`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| bufferIndex | Integer | no | The slot index of the buffer to update | Must match an index used in addBufferToWebSocket |

**Cross References:**
- `$API.ScriptWebView.addBufferToWebSocket$`
- `$API.ScriptWebView.setEnableWebSocket$`

---

## updateValueFromProcessorConnection

**Disabled:** property-deactivated
**Disabled Reason:** The processorId and parameterId properties are deactivated for ScriptWebView. WebView components cannot be connected to module parameters.
