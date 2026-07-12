TransportHandler::setEnableGrid(Integer shouldBeEnabled, Integer tempoFactor) -> undefined

Thread safety: SAFE
Enables or disables the high-precision grid timer at a specific musical tempo division. The `tempoFactor` is an index into the TempoSyncer note value table (0=1/1 through 18=1/64T). This is a global setting -- enabling the grid affects all TransportHandler instances. The grid must be enabled before `setOnGridChange` callbacks will fire.
Required setup:
  const var th = Engine.createTransportHandler();
  th.setEnableGrid(true, 11); // 1/16 note grid
Dispatch/mechanics: Validates `tempoFactor` with `isPositiveAndBelow(tempoFactor, numTempos)`, then delegates to `MasterClock::setClockGrid()`.
Pair with: setOnGridChange -- register a callback to receive grid events. setLocalGridMultiplier -- per-instance rate division. getGridLengthInSamples -- query resulting grid duration.
Anti-patterns:
  - The error message says "Use 1-18" but valid indices actually start at 0 (Whole note). Index 0 is valid.
Source:
  ScriptingApi.cpp:8626  setEnableGrid() -> MasterClock::setClockGrid()
