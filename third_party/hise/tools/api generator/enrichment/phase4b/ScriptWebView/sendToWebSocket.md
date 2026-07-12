ScriptWebView::sendToWebSocket(String id, NotUndefined data) -> undefined

Thread safety: UNSAFE
Sends data to the webview through the websocket connection. Dispatch varies by
data type: String sent directly, Buffer sent as raw binary float data, JSON
Object serialized to JSON string.
Required setup:
  const var wv = Content.addWebView("WebView1", 0, 0);
  wv.setEnableWebSocket(8080);
Dispatch/mechanics:
  if String: data->sendStringToWebsocket(id, str)
  if Buffer: data->sendDataToWebsocket(id, floatPtr, size * sizeof(float))
  if Object: JSON::toString(nd) -> data->sendStringToWebsocket(id, jsonStr)
Pair with:
  setEnableWebSocket -- must enable websocket first
  setWebSocketCallback -- receive messages from the JS side
  addBufferToWebSocket/updateBuffer -- for repeated buffer streaming (more efficient)
Source:
  ScriptingApiContent.cpp:6020  ScriptWebView::sendToWebSocket()
    -> three dispatch paths based on var type
