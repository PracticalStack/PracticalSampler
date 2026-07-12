Server::setServerCallback(Function callback) -> undefined

Thread safety: UNSAFE -- creates a new WeakCallbackHolder object, which involves heap allocation and reference counting.
Registers a callback invoked when the GET/POST request queue changes state. Fires when the queue drops to 1 item (server busy) or 0 items (server idle). Does NOT fire for every queue change -- only when the count is below 2. Useful for showing/hiding loading indicators. Callback is invoked on the Server Thread. Pass false to clear the callback. Download queue changes do NOT trigger this callback.
Callback signature: f(bool isActive)
Anti-patterns:
  - The callback only fires when the queue size is below 2 (0 or 1 items). If multiple requests are queued simultaneously, the callback fires once when the queue drains to 1 and once at 0 -- not for each intermediate step. Do NOT use this for tracking individual request completions.
  - Download queue changes do NOT trigger this callback. Use the per-download callback from downloadFile() to track download state.
Source:
  ScriptingApi.cpp  Server::setServerCallback()
    -> creates WeakCallbackHolder(callback, 1 arg)
    -> Server implements GlobalServer::Listener::queueChanged(int numItems)
    -> fires callback when numItems < 2, passing (numItems == 1) as isActive
