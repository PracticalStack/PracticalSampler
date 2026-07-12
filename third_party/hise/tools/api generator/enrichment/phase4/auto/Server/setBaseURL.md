Sets the base URL for all subsequent server calls and starts the background server thread. Call this once during `onInit` before any other Server method. All sub-URLs passed to `Server.callWithGET()`, `Server.callWithPOST()`, and `Server.downloadFile()` are appended to this base, so you only need to specify the relative path in each call.

> [!Warning:Required before any server requests] No server requests are processed until this method is called. If you skip it, calls to `Server.callWithGET()` or `Server.callWithPOST()` are silently lost.
