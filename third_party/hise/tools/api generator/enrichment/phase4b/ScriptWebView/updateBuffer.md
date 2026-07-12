ScriptWebView::updateBuffer(Integer bufferIndex) -> undefined

Thread safety: UNSAFE
Marks the buffer at the given index as dirty so it will be sent to the webview
on the next websocket communication cycle. The buffer must have been previously
registered via addBufferToWebSocket().
Pair with:
  addBufferToWebSocket -- register the buffer first
  setEnableWebSocket -- websocket must be enabled
Source:
  ScriptingApiContent.cpp:6047  ScriptWebView::updateBuffer()
    -> data->updateBuffer(bufferIndex)
