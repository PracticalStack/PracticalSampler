Broadcaster::setSendMessageForUndefinedArgs(bool shouldSendWhenUndefined) -> undefined

Thread safety: SAFE
Controls whether listener initialization proceeds when broadcaster args are undefined.
Only affects the initItem() path (when a new listener is added without attached sources).
Does NOT affect sendInternal() which always suppresses undefined args.
Pair with:
  addListener -- initialization affected by this flag
  sendSyncMessage / sendAsyncMessage -- NOT affected (always suppress undefined)
Anti-patterns:
  - Only affects initItem() path, NOT sendInternal() -- explicitly sent undefined args always suppressed.
  - No effect when sources are attached (initItem uses source's getInitialArgs instead).
Source:
  ScriptBroadcaster.cpp  setSendMessageForUndefinedArgs()
