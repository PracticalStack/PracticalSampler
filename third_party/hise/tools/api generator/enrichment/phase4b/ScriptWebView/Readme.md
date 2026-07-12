ScriptWebView (object)
Obtain via: Content.addWebView(name, x, y)

Embedded web browser component (via choc::WebView) for rendering HTML/CSS/JS
inside a HISE plugin interface. Provides bidirectional HiseScript-to-JS
communication via bound callbacks and function calls, plus TCP-based websocket
streaming for binary buffer data and JSON messages.

Complexity tiers:
  1. One-way display: setIndexFile, callFunction. Push data from HiseScript to
     JavaScript. Suitable for status displays, meters, or any view that only
     receives data.
  2. Bidirectional communication: + bindCallback, evaluate. Allow JavaScript to
     query HiseScript state or trigger actions (loading presets, playing notes).
  3. Binary streaming: + setEnableWebSocket, addBufferToWebSocket, updateBuffer,
     sendToWebSocket, setWebSocketCallback. High-throughput audio buffer
     streaming for real-time visualization.

Practical defaults:
  - Set enableCache to false during development for live reload, then true for
    export. When true and the root directory is inside the project folder, HISE
    automatically embeds web resources into the exported plugin.
  - Keep enablePersistence true (default) so callFunction/evaluate calls made
    before the webview exists are replayed when it appears. Handles the common
    case where onInit runs before the plugin UI is created.
  - Use reset() before setIndexFile() during development to clear stale cached
    state when iterating on web content.
  - Place web content (HTML/CSS/JS) under the project's Images folder so HISE's
    resource embedding picks it up automatically on export.

Common mistakes:
  - Calling setWebSocketCallback without setEnableWebSocket first -- throws
    script error "You have to enable the WebSocket before calling this method".
  - Passing a string path to setIndexFile instead of a File object -- throws
    script error. Use FileSystem API to resolve the path first.
  - Expecting callFunction to execute synchronously -- it dispatches
    asynchronously via MessageManager::callAsync. Use bindCallback for return
    values from JavaScript.
  - Placing web files outside the project folder with enableCache true -- HISE
    only embeds resources automatically when the root directory is inside the
    project folder. Files outside won't be included in exported plugins.

Example:
  const var wv = Content.addWebView("WebView1", 0, 0);
  wv.set("width", 600);
  wv.set("height", 400);

Methods (38):
  addBufferToWebSocket     bindCallback             callFunction
  changed                  evaluate                 fadeComponent
  get                      getAllProperties          getChildComponents
  getGlobalPositionX       getGlobalPositionY       getHeight
  getId                    getLocalBounds            getValue
  getWidth                 grabFocus                loseFocus
  reset                    sendRepaintMessage       sendToWebSocket
  set                      setConsumedKeyPresses    setControlCallback
  setEnableWebSocket       setHtmlContent           setIndexFile
  setKeyPressCallback      setLocalLookAndFeel      setPosition
  setStyleSheetClass       setStyleSheetProperty    setStyleSheetPseudoState
  setValue                 setWebSocketCallback     setZLevel
  showControl              updateBuffer
