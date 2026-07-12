Starts a WebSocket server on the specified port for bidirectional communication between HiseScript and the webview. Must be called before `setWebSocketCallback()` or `sendToWebSocket()`.

Pass a random port number (e.g. `parseInt(Math.random() * 65536)`) to allow each plugin instance to communicate independently with its own webview. A static port enables cross-instance communication across all plugin instances, but only one instance can bind the port at a time.

Typical data mapping over the WebSocket:

| Sent from JavaScript | Received in HISE |
|----------------------|------------------|
| String | String |
| `JSON.stringify(...)` | Parsed JSON object or array |
| `Float32Array` | Buffer |
