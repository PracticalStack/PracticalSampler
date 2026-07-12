## callFunction

**Examples:**

```javascript:call-function-complex-data
// Title: Sending complex data to JavaScript
// Context: Build a data model in HiseScript and push it to a JS function
// that populates an HTML interface (e.g. a list, table, or menu)

const var wv = Content.addWebView("WebView1", 0, 0);
wv.set("width", 600);
wv.set("height", 400);
wv.set("enablePersistence", true);

// Build a data array in HiseScript
const var items = [];

for (f in FileSystem.findFiles(FileSystem.getFolder(FileSystem.UserPresets), "*.preset", true))
{
    items.push({
        "name": f.toString(1),
        "path": f.toString(0).replace("\\", "/")
    });
}

// Push the entire array to a global JS function.
// With enablePersistence=true, this call is replayed when
// the webview is created (onInit runs before the UI appears)
wv.callFunction("updateItemList", items);
```
```json:testMetadata:call-function-complex-data
{
  "testable": false,
  "skipReason": "Requires active webview with updateItemList() defined in JS"
}
```

```javascript:call-function-stream-buffer
// Title: Streaming display buffer data on a timer
// Context: Read an oscilloscope or spectrum display buffer and send
// the encoded data to JavaScript for web-based visualization

const var wv = Content.addWebView("WebView1", 0, 0);
wv.set("width", 600);
wv.set("height", 400);

const var source = Synth.getDisplayBufferSource("ScriptFX1");
const var displayBuffer = source.getDisplayBuffer(1);

const var renderTimer = Engine.createTimerObject();

renderTimer.setTimerCallback(function()
{
    // Encode the buffer as a compact two-char-per-sample string
    local encoded = displayBuffer.getReadBuffer().toCharString(500, [-1, 1]);
    wv.callFunction("renderWaveform", encoded);
});

renderTimer.startTimer(30);
```
```json:testMetadata:call-function-stream-buffer
{
  "testable": false,
  "skipReason": "Requires active webview, display buffer source, and renderWaveform() in JS"
}
```

```javascript:call-function-cable
// Title: Driving JavaScript from a GlobalCable value change
// Context: Route a modulation signal (LFO, envelope, etc.) through a
// global cable and push the value to a JS animation function

const var wv = Content.addWebView("WebView1", 0, 0);
wv.set("width", 600);
wv.set("height", 400);

const var rm = Engine.getGlobalRoutingManager();
const var cable = rm.getCable("visualCable");

cable.registerCallback(function(value)
{
    // Scale the 0..1 cable value and push to JS
    wv.callFunction("updateAnimation", 3.0 + value);
}, AsyncNotification);
```
```json:testMetadata:call-function-cable
{
  "testable": false,
  "skipReason": "Requires active webview, global cable, and updateAnimation() in JS"
}
```

**Pitfalls:**
- `callFunction` is asynchronous (dispatched via `MessageManager::callAsync`). Do not expect the JavaScript function to have executed by the time the next HiseScript line runs.
- The target function must exist in the webview's global `window` scope. If the function is defined inside a module or closure, attach it to `window` explicitly in your JavaScript code.
- With `enablePersistence` set to `true`, all `callFunction` calls are recorded and replayed when a new webview instance is created. This is useful for initialization but means rapid repeated calls (e.g. from a timer) will accumulate. Timer-driven calls are replayed as the most recent value only when the function name matches.
