ScriptWebView::setWebSocketCallback(Function callbackFunction) -> undefined

Thread safety: UNSAFE
Registers a callback to receive incoming messages from the websocket connection.
The websocket must be enabled via setEnableWebSocket() first, otherwise a script
error is thrown.
Callback signature: f(var message)
Required setup:
  const var wv = Content.addWebView("WebView1", 0, 0);
  wv.setEnableWebSocket(8080);
Dispatch/mechanics:
  Creates WeakCallbackHolder(processor, 1 arg) -> incRefCount()
  Registers lambda on WebViewData: webSocketCallback.call1(v) on message receipt
  Distinct from bindCallback -- receives raw TCP messages, not JS function calls
Pair with:
  setEnableWebSocket -- must enable websocket first
  sendToWebSocket -- send data in the other direction
Anti-patterns:
  - Must call setEnableWebSocket() first -- throws script error:
    "You have to enable the WebSocket before calling this method"
Source:
  ScriptingApiContent.cpp:6050  ScriptWebView::setWebSocketCallback()
    -> new WeakCallbackHolder(processor, this, callbackFunction, 1)
    -> data->setWebSocketCallback(lambda)
