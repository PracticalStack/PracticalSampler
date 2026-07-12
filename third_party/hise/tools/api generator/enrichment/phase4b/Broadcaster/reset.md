Broadcaster::reset() -> undefined

Thread safety: UNSAFE -- calls sendInternal which acquires read lock, iterates targets, invokes callbacks
Dispatches original defaultValues to all listeners. Calls sendInternal() directly, bypassing
change detection, bypass check, and async path. lastValues is NOT updated.
For standard { args: [...] } format, defaults are undefined -> silently suppressed.
Dispatch/mechanics:
  Calls sendInternal(defaultValues) directly -- bypasses sendMessageInternal.
  No change detection, no bypass check, no async path, lastValues not updated.
  For standard { args: [...] } format, defaults are undefined -> silently suppressed.
Pair with:
  resendLastMessage -- to re-dispatch current values instead of defaults
  removeAllListeners / removeAllSources -- for actual cleanup
Anti-patterns:
  - For standard format broadcasters, reset() has no visible effect (undefined defaults suppressed).
  - Does NOT update lastValues -- reading bc.argName still returns the last sent value.
  - Bypasses the bypass check -- dispatches even when bypassed.
Source:
  ScriptBroadcaster.cpp  reset()
