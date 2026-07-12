ScriptWebView::setEnableWebSocket(Integer port) -> undefined

Thread safety: UNSAFE
Starts a TCP server on the specified port for websocket-style communication
between HiseScript and the webview. Must be called before setWebSocketCallback()
or sendToWebSocket(). Uses raw TCP with a custom framing protocol, not the
standard WebSocket protocol. Pass -1 for a random available port.
Dispatch/mechanics:
  data->setEnableWebsocket(port)
    -> creates TCPServer with ConnectionThread + CommunicationThread
Pair with:
  setWebSocketCallback -- register a callback for incoming messages
  sendToWebSocket -- send data to the webview
  addBufferToWebSocket -- register buffers for binary streaming

Typical data mapping:
  - String from JavaScript -> String in HISE
  - JSON.stringify(...) from JavaScript -> parsed JSON object / array in HISE
  - Float32Array from JavaScript -> Buffer in HISE

Source:
  ScriptingApiContent.cpp:6010  ScriptWebView::setEnableWebSocket()
    -> data->setEnableWebsocket(port)
