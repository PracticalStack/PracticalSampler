TransportHandler::getGridLengthInSamples() -> Double

Thread safety: SAFE
Returns the duration of one grid tick in samples, based on current BPM, global grid tempo setting, local grid multiplier, and sample rate. Accounts for the local multiplier -- if multiplier is 4, the returned length is 4x the base grid length.
Pair with: setEnableGrid -- sets the base grid rate. setLocalGridMultiplier -- multiplier is factored into the result.
Source:
  ScriptingApi.cpp:8719  getGridLengthInSamples() -> MasterClock::getCurrentClockGrid() -> TempoSyncer::getTempoFactor() * localGridMultiplier -> TempoSyncer::getTempoInSamples()
