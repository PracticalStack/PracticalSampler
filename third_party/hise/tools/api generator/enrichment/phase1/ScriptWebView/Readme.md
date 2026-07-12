# ScriptWebView -- Class Analysis

## Brief
Embedded web browser component for rendering HTML/CSS/JS with bidirectional HiseScript communication and websocket data streaming.

## Purpose
ScriptWebView embeds a native web browser (via choc::WebView) into the HISE plugin interface, enabling HTML/CSS/JS-based UI rendering alongside native HISE components. It provides bidirectional communication through bound callbacks (JS-to-HiseScript) and function calls/evaluate (HiseScript-to-JS), plus a TCP-based websocket system for streaming binary buffer data and JSON messages. The underlying WebViewData is a singleton-per-name that persists across script recompilations, with automatic call replay for consistent initialization of new webview instances.

## Details

### Architecture

ScriptWebView is a thin scripting wrapper around `WebViewData`, which lives on the `MainController` (via `GlobalScriptCompileBroadcaster`). Multiple ScriptWebView instances with the same component name share the same WebViewData. This separation means the web content state survives script recompilation -- only the HiseScript callback bindings are re-established.

### Content Loading Modes

WebViewData supports three resource resolution modes:

| Mode | Description | Use Case |
|------|-------------|----------|
| FileBased | Reads HTML/CSS/JS/images from a root directory on disk | Development -- enables live reload |
| Embedded | Resources baked into a ValueTree | Exported plugins -- no file I/O needed |
| Hardcoded | Inline HTML string | Simple content via `setHtmlContent()` |

Use `setIndexFile()` with a File object for file-based mode, or `setHtmlContent()` for inline HTML. For exported plugins, HISE automatically embeds file-based resources if the root directory is inside the project folder and caching is enabled.

### Communication Channels

There are two independent communication systems:

**1. Callback binding** -- See `bindCallback()`, `callFunction()`, and `evaluate()` for the JS-to-HiseScript and HiseScript-to-JS callback API.

**2. WebSocket (TCP-based binary/string streaming)** -- See `setEnableWebSocket()` to start the TCP server, `sendToWebSocket()` and `addBufferToWebSocket()`/`updateBuffer()` for sending data, and `setWebSocketCallback()` for receiving messages. The websocket system uses raw TCP with a custom framing protocol, not the standard WebSocket protocol.

### Scale Factor Handling

When `scaleFactorToZoom` is true (default), the system/host DPI scale factor is applied as browser zoom rather than native scaling. The component wrapper listens to both `GlobalSettingManager::ScaleFactorListener` and `ZoomableViewport::ZoomListener` to update the browser zoom dynamically.

## obtainedVia
`Content.addWebView(name, x, y)`

## minimalObjectToken
wv

## Constants
None.

## Dynamic Constants
None.

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| `wv.setWebSocketCallback(fn);` | `wv.setEnableWebSocket(port); wv.setWebSocketCallback(fn);` | Must enable the websocket before registering a callback, otherwise a script error is thrown. |
| `wv.setIndexFile("/path/to/index.html");` | `wv.setIndexFile(FileSystem.getFolder(FileSystem.AudioFiles).getChildFile("webview/index.html"));` | setIndexFile requires a File object, not a string path. Passing a string causes a script error. |

## codeExample
```javascript
const var wv = Content.addWebView("WebView1", 0, 0);
wv.set("width", 600);
wv.set("height", 400);
```

## Alternatives
Use ScriptPanel for native HISE graphics drawing instead of HTML/CSS rendering. Use ScriptFloatingTile for embedding built-in HISE widgets rather than custom web content.

## Related Preprocessors
None.

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: Both ordering dependencies (setEnableWebSocket before setWebSocketCallback, and setConsumedKeyPresses before setKeyPressCallback) already produce clear script errors at runtime, so no additional parse-time diagnostic is needed.
