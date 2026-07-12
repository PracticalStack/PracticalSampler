TransportHandler::setSyncMode(Integer syncMode) -> undefined

Thread safety: SAFE
Sets the sync mode for the global master clock, controlling how the internal clock interacts with the external DAW clock. This is a global setting affecting all TransportHandler instances. Use TransportHandler constants as the argument: Inactive (0), ExternalOnly (1), InternalOnly (2), PreferInternal (3), PreferExternal (4), SyncInternal (5).
Required setup:
  const var th = Engine.createTransportHandler();
  th.setSyncMode(th.PreferExternal);
Dispatch/mechanics: Directly sets `MasterClock::setSyncMode()` with an unchecked enum cast.
Pair with: startInternalClock/stopInternalClock -- internal clock only functions in modes that allow it. setLinkBpmToSyncMode -- links BPM source to the active sync mode.
Source:
  ScriptingApi.cpp:8699  setSyncMode() -> MasterClock::setSyncMode()
