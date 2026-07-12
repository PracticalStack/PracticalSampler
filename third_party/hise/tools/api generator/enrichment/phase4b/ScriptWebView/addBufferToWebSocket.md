ScriptWebView::addBufferToWebSocket(Integer bufferIndex, Buffer buffer) -> undefined

Thread safety: UNSAFE
Registers a buffer at a specific index for efficient repeated streaming through
the websocket connection. Call updateBuffer() with the same index to mark it
dirty and trigger a send on the next communication cycle.
Required setup:
  const var wv = Content.addWebView("WebView1", 0, 0);
  wv.setEnableWebSocket(8080);
Dispatch/mechanics:
  Casts var to VariantBuffer -> WebViewData::addBufferToWebsocket(index, bufferPtr)
  Registers a BufferSlot in the TCP server for indexed binary streaming
Pair with:
  updateBuffer -- marks the registered buffer as dirty to trigger send
  setEnableWebSocket -- must enable websocket before buffer streaming works
Anti-patterns:
  - Passing a non-Buffer argument silently does nothing -- no error reported
Source:
  ScriptingApiContent.cpp:6040  ScriptWebView::addBufferToWebSocket()
    -> data->addBufferToWebsocket(bufferIndex, b)
