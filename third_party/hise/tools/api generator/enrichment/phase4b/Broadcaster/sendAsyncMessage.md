Broadcaster::sendAsyncMessage(var args) -> undefined

Thread safety: UNSAFE -- acquires SimpleReadWriteLock write lock, posts job to JavascriptThreadPool
Sends message asynchronously to all listeners via sendMessageInternal(args, false).
Change detection suppresses duplicates. Rapid sends coalesced via asyncPending atomic
unless queue mode is enabled. With queue mode, each send captures its own snapshot.
Dispatch/mechanics:
  sendMessageInternal -> change detection -> write lock on lastValues
  -> posts HiPriorityCallbackExecution to JavascriptThreadPool
  -> asyncPending atomic coalesces rapid sends (unless queue mode)
  -> with queue: each send captures own lastValues snapshot.
Pair with:
  sendSyncMessage -- synchronous alternative
  setEnableQueue -- to ensure every message is delivered
  sendMessageWithDelay -- delayed async send
Anti-patterns:
  - Without queue mode, rapid sends are coalesced -- only latest values dispatched.
  - Undefined args silently suppress the message (no callbacks, no error).
  - Change detection suppresses duplicates -- use resendLastMessage if needed.
Source:
  ScriptBroadcaster.cpp:3662  sendMessageInternal(args, false)
