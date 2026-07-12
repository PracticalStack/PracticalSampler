Server::getPendingCalls() -> Array

Thread safety: UNSAFE -- iterates the pending callbacks list and constructs a new Array with heap allocation.
Returns a snapshot array of all pending GET/POST request objects currently queued on the Server Thread. Completed requests are not included. The array is not live-updated as requests complete.
Pair with:
  callWithGET / callWithPOST -- methods that add to the pending queue
  getPendingDownloads -- equivalent for download queue
Source:
  ScriptingApi.cpp  Server::getPendingCalls()
    -> iterates WebThread::pendingCallbacks
    -> constructs Array of PendingCallback objects
