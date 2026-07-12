Broadcaster::setReplaceThisReference(bool shouldReplaceThisReference) -> undefined

Thread safety: SAFE
Controls whether the obj parameter from addListener replaces this in the callback.
Default is true. NOTE: In current implementation, this flag is stored but never consulted
during dispatch -- callSync always uses obj as this. Setting to false is a no-op.
Pair with:
  addListener -- affected by this setting (in theory)
Anti-patterns:
  - [BUG] Setting to false has no effect -- replaceThisReference member is never read by dispatch code.
Source:
  ScriptBroadcaster.cpp  setReplaceThisReference()
