Registers a callback that receives incoming messages from the WebSocket connection. The WebSocket must be enabled via `setEnableWebSocket()` before calling this method. The callback receives a single parameter whose type depends on what the JavaScript side sent:

- A String when a string is sent
- A JSON object when a stringified JSON object is sent (via `JSON.stringify()` in JavaScript)
- A Buffer when a `Float32Array` is sent

Messages arrive in the order they were sent. Use `HiseWebSocketServer.send()` on the JavaScript side to transmit data back to HISE.
