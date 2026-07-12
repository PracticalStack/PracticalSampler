TransportHandler::setLinkBpmToSyncMode(Integer shouldPrefer) -> undefined

Thread safety: SAFE
When enabled, the BPM source (internal vs external) is automatically linked to the current sync mode. Modes that prefer internal (InternalOnly, PreferInternal, SyncInternal) use internal BPM; ExternalOnly and PreferExternal use host BPM. This is a global setting on the MasterClock.
Required setup:
  const var th = Engine.createTransportHandler();
  th.setLinkBpmToSyncMode(true);
Pair with: setSyncMode -- determines which BPM source is selected.
Source:
  ScriptingApi.cpp:8709  setLinkBpmToSyncMode() -> MasterClock::setLinkBpmToSyncMode()
