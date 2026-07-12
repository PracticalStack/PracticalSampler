Broadcaster::setBypassed(bool shouldBeBypassed, bool sendMessageIfEnabled, bool async) -> undefined

Thread safety: UNSAFE -- when unbypassing with sendMessageIfEnabled, calls resendLastMessage which acquires locks and may invoke callbacks
Controls bypass state. Bypassed broadcaster stores new lastValues but skips dispatch.
When unbypassing with sendMessageIfEnabled=true, resends last values.
WARNING: async parameter name is inverted -- true = synchronous, false = asynchronous.
Use SyncNotification/AsyncNotification constants.
Dispatch/mechanics:
  Sets bypassed bool. When unbypassing with sendMessageIfEnabled:
  calls resendLastMessage(async).
  While bypassed: sendMessageInternal stores lastValues but skips dispatch.
Pair with:
  isBypassed -- read the current state
  resendLastMessage -- called internally when unbypassing
Anti-patterns:
  - async parameter is inverted: true = sync, false = async. Use notification constants.
  - Bypassed broadcaster still updates lastValues -- values not discarded.
  - Only most recent values sent on unbypass, not history.
Source:
  ScriptBroadcaster.cpp  setBypassed()
