# ScriptWebView -- Project Context

## Project Context

### Real-World Use Cases
- **Custom HTML-based UI widgets**: A plugin that needs a preset browser, data table, or other complex interactive element beyond what native HISE components offer can embed an HTML/CSS/JS interface inside a ScriptWebView. The webview handles rendering and user interaction while HiseScript manages the audio engine state, with bidirectional communication bridging the two.
- **3D/WebGL visualization**: A plugin that wants to render 3D graphics (e.g. Three.js scenes, PlayCanvas apps) embeds the rendering engine in a ScriptWebView and drives it from HiseScript via `callFunction` and `bindCallback`. MIDI events and modulation signals can control visual parameters in real time.
- **Audio data visualization**: A plugin that needs waveform displays, spectrum analyzers, or other audio visualizations beyond the built-in components can stream display buffer data to a web-based renderer via `callFunction` on a timer, or use the websocket system for higher-throughput binary streaming.

### Complexity Tiers
1. **One-way display** (simplest): `setIndexFile` + `callFunction` to push data from HiseScript to JavaScript. Suitable for status displays, meters, or any view that only receives data.
2. **Bidirectional communication**: Add `bindCallback` to allow JavaScript to query HiseScript state or trigger actions (loading presets, playing notes). This covers interactive custom UIs.
3. **Binary streaming**: Add `setEnableWebSocket` + `addBufferToWebSocket` + `updateBuffer` for high-throughput audio buffer streaming. Needed for real-time audio visualization at frame rate.

### Practical Defaults
- Set `enableCache` to `false` during development for live reload, then `true` for export. When `enableCache` is `true` and the root directory is inside the project folder, HISE automatically embeds the web resources into the exported plugin.
- Set `enablePersistence` to `true` (the default) so that `callFunction` and `evaluate` calls made before the webview exists are replayed when it appears. This handles the common case where `onInit` runs before the plugin UI is created.
- Use `reset()` before `setIndexFile()` during development to clear stale cached state when iterating on web content.
- Place web content (HTML/CSS/JS) in a subdirectory under the project's `Images` folder so that HISE's resource embedding picks it up automatically on export.

### Integration Patterns
- `ScriptWebView.bindCallback()` -> `Engine.loadUserPreset()` -- JavaScript triggers preset loading in HISE via a bound callback.
- `ScriptWebView.callFunction()` <- `UserPresetHandler.setPostCallback()` -- When a preset loads in HISE, the post-callback pushes the new state to JavaScript for UI synchronization.
- `ScriptWebView.callFunction()` <- `GlobalCable.registerCallback()` -- A global routing cable's value change drives a JavaScript function call, connecting modulation signals to web-based visualization.
- `DisplayBuffer.getReadBuffer()` -> `ScriptWebView.callFunction()` via `Timer` -- A timer periodically reads a display buffer and sends the encoded data to JavaScript for waveform/spectrum rendering.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| Place web files outside the project folder and set `enableCache` to `true` | Place web files inside the project's `Images` folder | HISE only embeds web resources automatically when the root directory is inside the project folder. Files outside the project won't be included in exported plugins. |
| Call `callFunction` expecting synchronous execution | Treat `callFunction` as fire-and-forget; use `bindCallback` for return values | `callFunction` dispatches asynchronously to the message thread. To get data back from JavaScript, bind a callback that JavaScript calls with the result. |
