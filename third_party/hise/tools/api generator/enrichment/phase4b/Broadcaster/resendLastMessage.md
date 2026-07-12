Broadcaster::resendLastMessage(var isSync) -> undefined

Thread safety: UNSAFE -- delegates to sendMessageInternal which acquires locks, may post async jobs
Re-dispatches current lastValues to all listeners, bypassing change detection.
Use SyncNotification or AsyncNotification constants for the isSync parameter.
Commonly used after unbypassing or when external state changed.
Dispatch/mechanics:
  Sets scoped forceSend = true via ScopedValueSetter.
  Calls sendMessageInternal(var(lastValues), isSync).
  Bypasses change detection but NOT undefined check or bypass check.
Pair with:
  sendSyncMessage / sendAsyncMessage -- normal send paths
  setBypassed -- calls this internally when unbypassing with sendMessageIfEnabled
Anti-patterns:
  - Does NOT bypass the undefined-argument check -- if lastValues contain undefined, message is suppressed.
  - Does NOT bypass the bypass check -- bypassed broadcaster still blocks dispatch.
Source:
  ScriptBroadcaster.cpp  resendLastMessage()
