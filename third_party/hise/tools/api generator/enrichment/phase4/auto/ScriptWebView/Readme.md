# ScriptWebView

ScriptWebView embeds a native web browser into the plugin interface, so you can build part of your UI with HTML, CSS, and JavaScript while keeping the data model and audio logic in HiseScript. Use it when native ScriptPanel drawing would be too limited, or when you want to reuse an existing web-based UI.

Two communication channels connect HiseScript and the browser:

1. **Direct communication** - bind HiseScript functions that JavaScript calls via a Promise pattern, or call JavaScript functions from HiseScript. Best for simple data (text, small JSON objects) with no additional setup.
2. **WebSocket streaming** - a standard WebSocket server for high-throughput bidirectional transfer of strings, JSON objects, and binary audio buffers. Best for large data blobs; also allows developing the webview in a standalone browser.

| Topic | WebSocket | Direct communication |
| --- | --- | --- |
| **Data size** | Large data blobs (audio buffers, images) | Simple data (text, small JSON objects) |
| **Development** | Can develop the webview in a standalone browser | Tightly coupled to HISE |
| **Setup** | Additional client-side JavaScript required | Bind and call functions directly |

```js
const var wv = Content.addWebView("WebView1", 0, 0);
```

Choose direct communication for normal UI state updates and button-like interactions. Use the WebSocket path only when you need large payloads, browser-based development outside HISE, or continuous streaming data.

Several component properties control WebView-specific behaviour:

| Property | Description |
| --- | --- |
| `rootDirectory` | The root folder for resolving all resource URLs (also settable in the properties panel) |
| `indexFile` | Relative path from the root directory to the initial HTML file (also settable in the properties panel) |
| `enableCache` | Caches resources instead of loading from disk. Must be enabled to embed resources in exported plugins |
| `enablePersistence` | Replays all HiseScript-to-JS communication when a new webview instance is created |
| `enableDebugMode` | Enables browser developer tools |
| `scaleFactorToZoom` | Applies the host DPI scale factor as browser zoom (default: true) |

The persistence system handles the common plugin lifecycle where the webview is destroyed and recreated when the plugin window closes and reopens. Treat the webview as a visual frontend - HISE should remain the source of truth for state, processing, and preset data.

For WebSocket communication, the JavaScript side connects using the built-in `HiseWebSocketServer` helper. Include `<script src="hisewebsocket-min.js"></script>` in your HTML header, create a `HiseWebSocketServer` with the chosen port, then use `addEventListener()` and `send()` for message exchange. Random ports isolate each plugin instance; a fixed port can be shared across instances.

> [!Tip:Native handle limits layering and blending] **Native handle limitation:** The webview uses a native OS browser handle placed on top of the plugin interface. Alpha blending, masking, and layering other UI components in front of or behind the webview are not supported. During development in the Interface Designer, the scale factor may appear incorrect and the webview may overlap editor components.

> [!Tip:Disable cache for dev, enable for export] Keep `enableCache` off during development for live-reload behaviour, and enable it for export. When enabled and the root directory is inside the project folder, HISE automatically embeds the web resources into the exported plugin.

## Common Mistakes

- **Use project-relative web folder path**
  **Wrong:** `wv.setIndexFile("/path/to/index.html");`
  **Right:** `wv.setIndexFile(FileSystem.getFolder(FileSystem.AudioFiles).getChildFile("webview/index.html"));`
  *`setIndexFile` requires a File object from the FileSystem API, not a string path.*

- **callFunction is asynchronous**
  **Wrong:** Call `callFunction` and expect synchronous execution or a return value.
  **Right:** Use `callFunction` as fire-and-forget; use `bindCallback` when JavaScript needs to return data to HiseScript.
  *`callFunction` dispatches asynchronously. To receive data back from JavaScript, bind a callback that JavaScript calls with the result.*

- **Enable enableCache for exported plugins**
  **Wrong:** Place web files outside the project folder with `enableCache` set to `true`.
  **Right:** Place web files inside the project folder (e.g. under the `Images/` directory).
  *HISE only embeds web resources when the root directory is inside the project folder.*

- **Set WebSocket callback before creating socket**
  **Wrong:** `wv.setWebSocketCallback(fn);` without calling `setEnableWebSocket` first.
  **Right:** `wv.setEnableWebSocket(port); wv.setWebSocketCallback(fn);`
  *The WebSocket must be enabled before registering a callback or sending data.*
