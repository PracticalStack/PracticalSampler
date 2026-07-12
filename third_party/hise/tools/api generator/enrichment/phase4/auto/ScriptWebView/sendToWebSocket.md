Sends data to the webview through the WebSocket connection. The data type determines the transfer format: strings are sent directly, JSON objects are serialised to a JSON string, and Buffer objects are sent as raw binary float data (received as `Float32Array` on the JavaScript side).

The `id` parameter coalesces rapid calls - if multiple calls use the same ID before the previous one is delivered, only the most recent data is sent. The ID can also be a stringified JSON object to attach metadata alongside a buffer payload.

> [!Warning:Messages coalesced by ID] Calls are coalesced by ID. If you send multiple updates with the same ID in quick succession, intermediate values may be dropped. Use unique IDs when every message must be delivered.
