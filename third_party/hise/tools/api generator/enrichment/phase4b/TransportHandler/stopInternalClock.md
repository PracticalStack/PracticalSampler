TransportHandler::stopInternalClock(Integer timestamp) -> undefined

Thread safety: SAFE
Stops the internal master clock at the given sample timestamp offset within the current audio block. This is a global operation affecting the shared MasterClock. If called during audio rendering, immediately processes grid and transport callbacks for the current block.
Required setup:
  const var th = Engine.createTransportHandler();
  th.stopInternalClock(0);
Dispatch/mechanics: Calls `MasterClock::changeState(timestamp, internalClock=true, startPlayback=false)`. If inside audio rendering and state changed, calls `processAndCheckGrid()`, creates internal playhead, and calls `handleTransportCallbacks()`.
Pair with: startInternalClock -- start counterpart. setOnTransportChange -- receives the resulting play state change.
Source:
  ScriptingApi.cpp:8684  stopInternalClock() -> MasterClock::changeState() -> processAndCheckGrid() -> handleTransportCallbacks()
