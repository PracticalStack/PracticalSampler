Checks whether the system has an active internet connection. Returns `true` if online, `false` otherwise. Use this as a gate before operations that need explicit connectivity feedback - for example, before offering a retry option or displaying an error panel.

> [!Warning:Blocks up to 20 seconds when offline] This method blocks for up to 20 seconds when the system is offline. Do not call it routinely before every server request. The request callback already receives `status == 0` on timeout, which is a non-blocking way to detect connection failures.
