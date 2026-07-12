TransportHandler::stopInternalClockOnExternalStop(Integer shouldStop) -> undefined

Thread safety: SAFE
Configures whether the internal clock should automatically stop when the external (DAW) clock stops. This is a global setting on the MasterClock. Useful with SyncInternal mode to prevent the internal clock from continuing after the DAW stops.
Required setup:
  const var th = Engine.createTransportHandler();
  th.stopInternalClockOnExternalStop(true);
Pair with: setSyncMode -- behavior is most relevant in PreferExternal and SyncInternal modes. startInternalClock/stopInternalClock -- the internal clock that gets stopped.
Source:
  ScriptingApi.cpp:8467  stopInternalClockOnExternalStop() -> MasterClock::setStopInternalClockOnExternalStop()
