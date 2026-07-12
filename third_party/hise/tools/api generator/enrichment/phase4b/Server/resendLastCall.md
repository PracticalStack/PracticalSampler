Server::resendLastCall() -> Integer

Thread safety: UNSAFE -- calls isOnline() internally, which blocks the scripting thread synchronously for up to 20 seconds. Also adds to the pending callbacks array and constructs String objects.
Re-queues the most recent GET or POST request. First checks internet connectivity via isOnline() (blocking), then re-adds the last PendingCallback to the WebThread queue. Returns true if successfully re-queued, false if offline, no previous call exists, or the callback became invalid due to script recompilation. The "last call" is the single most recent callWithGET/callWithPOST request.
Pair with:
  isOnline -- called internally before re-queuing
  callWithGET / callWithPOST -- the methods whose last call can be resent
Anti-patterns:
  - Do NOT call in tight loops or performance-sensitive contexts -- blocks for up to 20 seconds via internal isOnline() check
  - After script recompilation, the last call's WeakCallbackHolder becomes invalid. resendLastCall() returns false silently -- the request is not re-sent.
  - Only the single most recent GET or POST call is tracked. If multiple calls are made, only the last one can be resent.
Source:
  ScriptingApi.cpp  Server::resendLastCall()
    -> isOnline() (blocking connectivity check)
    -> GlobalServer::resendLastCallback()
      -> checks lastCall != nullptr and WeakCallbackHolder still valid
      -> lastCall->reset() clears timing/response fields
      -> pendingCallbacks.add(lastCall), notify WebThread
