## bindCallback

**Examples:**

```javascript:bind-callback-query
// Title: Querying HiseScript state from JavaScript
// Context: Bind a callback that JavaScript calls via the Promise pattern
// to read a component value. The JS side calls getRotation().then(v => use(v))

const var wv = Content.addWebView("WebView1", 0, 0);
wv.set("width", 600);
wv.set("height", 400);

const var rotationKnob = Content.addKnob("RotationKnob", 0, 410);

// The JS side calls: getRotation(null).then(speed => { animate(speed); })
wv.bindCallback("getRotation", function(args)
{
    return rotationKnob.getValue() * 0.05;
});
```
```json:testMetadata:bind-callback-query
{
  "testable": false,
  "skipReason": "Requires active webview with JS content to trigger the bound callback"
}
```

```javascript:bind-callback-action
// Title: Triggering a HiseScript action from JavaScript
// Context: A custom HTML preset browser calls back into HiseScript
// to load a user preset when the user selects one

const var wv = Content.addWebView("WebView1", 0, 0);
wv.set("width", 600);
wv.set("height", 400);

// JS calls: loadPreset([filePath]).then(() => { updateUI(); })
wv.bindCallback("loadPreset", function(args)
{
    // args is an array -- the first element is the file path string
    Engine.loadUserPreset(FileSystem.fromAbsolutePath(args[0]));
});
```
```json:testMetadata:bind-callback-action
{
  "testable": false,
  "skipReason": "Requires active webview with JS content and valid user presets"
}
```

```javascript:bind-callback-midi
// Title: Triggering MIDI from a JavaScript event
// Context: A web-based game or interactive element triggers a note
// when a JS event fires (e.g. a collision, button click, animation end)

const var wv = Content.addWebView("WebView1", 0, 0);
wv.set("width", 600);
wv.set("height", 400);

// JS calls: onGameEvent(null).then(() => {})
wv.bindCallback("onGameEvent", function(args)
{
    Synth.playNote(64, 127);
});
```
```json:testMetadata:bind-callback-midi
{
  "testable": false,
  "skipReason": "Requires active webview with JS content to trigger the bound callback"
}
```

**Pitfalls:**
- The bound callback executes synchronously on the webview's callback thread (via `WeakCallbackHolder::callSync`). Avoid long-running operations inside the callback -- defer heavy work to a timer or background task.
- The JavaScript side must use the Promise pattern: `callbackId(args).then(result => { ... })`. Calling the bound function without `.then()` still works but discards the return value.
